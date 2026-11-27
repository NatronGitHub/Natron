#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2026 The Natron developers
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

"""Soak test of disk renders with NatronRenderer: a render must never stall (#248).

Usage: run_render_soak_test.py /path/to/NatronRenderer [options] [--debugger]

Each run starts NatronRenderer on one graph of render_soak_graph.py (the "writers" graph: four Write nodes
after generators, a Read of an image sequence, Reformat, expressions, motion blur, Merge, FrameHold, Blur and
DiskCache; the "sequential" graph: a plug-in rendered in frame order, and WriteFFmpeg; the "baymax" graph: the
four configurations of Natron-Tests' BayMax project, rendered from the command line as runTests.sh does), driven one
of two ways:
- cli: "NatronRenderer -w <Write> <file> ... project.ntp first-last", all the Write nodes rendered at once;
- python: "NatronRenderer -t render_soak_graph.py", which renders with app.render(): one Write node in many
  short renders, the others together, and the last one aborted with natron.quit() from a callback;
  a second NatronRenderer then renders that Write node again (restart).
with one threading configuration (--setting, see CONFIGS): the runs go through the combinations in turn. Each run
has its own disk cache, seeded with the OFX plug-in cache of the setup, and is a new NatronRenderer (its own RAM
cache); Grade nodes after the generators and the Read nodes ("RunGain", "SoakRunGain" in BayMax) change slightly
from run to run, so that no image can come from an earlier run. The cli runs use render statistics (-s): each frame
must show a cache miss of a frame-varying node (PROBES), or the run fails. The Python driver renders each frame
once per Write node, except one pass that re-renders frames from the cache on purpose (labelled in its output).
Renders in the Natron GUI are not covered.

A watchdog follows the progress: the "<Write> ==> Frame: N" lines of NatronRenderer and the files written in the
output directory. When nothing progresses for --stall-timeout seconds, it prints the backtrace of every thread
(by sending SIGABRT to NatronRenderer under natron-lldb.sh, else by attaching lldb or gdb), saves it in the
output directory, kills NatronRenderer, and the test fails. A run also fails if a frame is missing, or if
NatronRenderer exits with an unexpected status.

Options:
  --frames N         frames per Write node (default 120)
  --runs N           number of runs (default: one per graph and driver)
  --duration S       keep starting runs for S seconds (instead of --runs)
  --stall-timeout S  seconds without progress that count as a stall (default 120)
  --graphs LIST      comma-separated graphs: writers, sequential, baymax (default writers,sequential)
  --drivers LIST     comma-separated drivers (default cli,python)
  --configs LIST     comma-separated threading configurations (default: all, see CONFIGS)
  --size WxH         size of the rendered images (default 320x180)
  --out DIR          keep the logs, backtraces and summary in DIR (default: a temporary directory, removed)
  --ofx-path DIR     add DIR to OFX_PLUGIN_PATH (the test plug-ins, for net.natron.test.Sequential)
  --natron-tests DIR a Natron-Tests checkout, with BayMax/Robot (runTests.sh downloads it), for --graphs baymax
  --pyplug-path DIR  add DIR to NATRON_PLUGIN_PATH (BayMax needs the LightWrap PyPlug of Gui/Resources/PyPlugs)
  --idiff PATH       idiff, to compare the first frame of each BayMax configuration with its reference (default:
                     idiff on PATH; no comparison without it)
  --debugger         run NatronRenderer under natron-lldb.sh (or set NATRON_TEST_WRAPPER, see natronrunner.py)

Exit status: 0 on success, 77 if skipped (no OpenColorIO configuration, or the plug-ins of openfx-misc and
openfx-io are not on OFX_PLUGIN_PATH), 1 on failure (including a graph whose project could not be saved). A stall
prints "SOAK STALL". SOAK_HANG_FRAME=<n> makes the Python driver hang at frame n, to check the watchdog.
"""

import argparse
import itertools
import json
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import threading
import time

import natronrunner

HERE = os.path.dirname(os.path.abspath(__file__))
GRAPH_SCRIPT = os.path.join(HERE, "render_soak_graph.py")
ABORT_STATUS = 3  # natron.quit() status of an aborted render (render_soak_graph.py)
# threading configurations: name -> --setting commands. Single-threaded and the "parallel renders" values that
# the reports of #248 mention (1, 2, 4, 8), the thread pool off, and the RAM settings of the cache
# (maxRAMPercent: the cache evicts images all the time; unreachableRAMPercent: "System RAM to keep free").
CONFIGS = {
    "default": [],
    "1thread": ["noRenderThreads=1"],
    "nothreads": ["noRenderThreads=-1"],
    "parallel2": ["nParallelRenders=2"],
    "parallel4": ["nParallelRenders=4"],
    "parallel8": ["nParallelRenders=8", "nThreadsPerEffect=2"],
    "nopool": ["useThreadPool=False"],
    "lowram": ["maxRAMPercent=1"],
    "keepfree": ["unreachableRAMPercent=90"],
    # the combinations of #248 (ltsaber86: 5 or 6 parallel renders and 5-10 % of the RAM kept free stall)
    "parallel6keepfree10": ["nParallelRenders=6", "unreachableRAMPercent=10"],
    "parallel8lowram": ["nParallelRenders=8", "maxRAMPercent=1"],
}
DEBUGGER_TIMEOUT = 120  # seconds for a debugger to print the backtraces of a stalled NatronRenderer
INPUT_FRAMES = 50  # frames of the sequence that the Read node of the writers graph reads (it loops)
# The nodes (by name prefix) of each graph whose render statistics (NatronRenderer -s) must show a cache miss at each
# frame, in the statistics of at least one Write node: the frame was rendered, not taken from a cache. Frame-varying
# nodes; in BayMax, the misses of all the SoakRunGain nodes are added up (the one after its still image hits the cache
# after the first frame, the one after the Robot sequence misses at every frame).
PROBES = {"writers": "Grade", "sequential": "Grade", "baymax": "SoakRunGain"}
# Natron-Tests' BayMax project: its configurations, as runTests.sh reads them, and the idiff options it uses for BayMax
BAYMAX_CONFS = ("conf", "conf2", "conf3", "conf4")
BAYMAX_IDIFF_OPTS = ["-warn", "0.001", "-fail", "0.008", "-failpercent", "0.2", "-hardfail", "0.08", "-abs",
                     "-scale", "30"]
# Run by NatronRenderer -l: on a BayMax project, and on the saved projects of the other graphs
BAYMAX_ONLOAD = os.path.join(HERE, "render_soak_baymax_onload.py")
GAIN_ONLOAD = os.path.join(HERE, "render_soak_gain_onload.py")


def parse_args(argv):
    """The options of the command line (see the module docstring)."""
    parser = argparse.ArgumentParser(description="Soak test of disk renders with NatronRenderer (#248).")
    parser.add_argument("renderer", help="the NatronRenderer executable")
    parser.add_argument("--frames", type=int, default=120)
    parser.add_argument("--runs", type=int, default=0)
    parser.add_argument("--duration", type=float, default=0.)
    parser.add_argument("--stall-timeout", type=float, default=120.)
    parser.add_argument("--graphs", default="writers,sequential")
    parser.add_argument("--drivers", default="cli,python")
    parser.add_argument("--configs", default=",".join(CONFIGS))
    parser.add_argument("--size", default="320x180")
    parser.add_argument("--out", default="")
    parser.add_argument("--ofx-path", default="")
    parser.add_argument("--natron-tests", default="",
                        help="a Natron-Tests checkout with BayMax/Robot, for the baymax graph")
    parser.add_argument("--pyplug-path", default="",
                        help="add this directory to NATRON_PLUGIN_PATH (BayMax uses the LightWrap PyPlug: "
                             "Gui/Resources/PyPlugs of the sources, for a build tree)")
    parser.add_argument("--idiff", default="",
                        help="idiff, to compare frames of BayMax with the references (default: on PATH)")
    parser.add_argument("--debugger", action="store_true")
    opts = parser.parse_args(argv)
    unknown = [c for c in opts.configs.split(",") if c not in CONFIGS]
    if unknown:
        parser.error("unknown configurations: " + ",".join(unknown))
    return opts


class Watchdog:
    """Runs one NatronRenderer command and follows its progress (output lines and files)."""

    def __init__(self, cmd, env, out_dir, stall_timeout, log_path, wrapped):
        self.cmd = cmd
        self.env = env
        self.out_dir = out_dir
        self.stall_timeout = stall_timeout
        self.log_path = log_path
        self.wrapped = wrapped
        self.lines = []
        self.last_progress = time.time()
        self.longest_gap = 0.
        self.lock = threading.Lock()

    def _read(self, stream):
        with open(self.log_path, "a", encoding="utf-8", errors="replace") as log:
            for raw in iter(stream.readline, b""):
                line = raw.decode(errors="replace").rstrip("\r\n")
                log.write(line + "\n")
                with self.lock:
                    self.lines.append(line)
                    if " ==> Frame: " in line or line.startswith("SOAK "):
                        self._progress()

    def output_lines(self):
        """A copy of the lines that the command printed so far."""
        with self.lock:
            return list(self.lines)

    def _progress(self):
        now = time.time()
        self.longest_gap = max(self.longest_gap, now - self.last_progress)
        self.last_progress = now

    def _files(self):
        state = []
        for name in os.listdir(self.out_dir):
            try:
                state.append((name, os.path.getsize(os.path.join(self.out_dir, name))))
            except OSError:
                pass
        return sorted(state)

    def run(self):
        """Returns (exit status, stalled, backtrace text). The status is None if NatronRenderer was killed."""
        with open(self.log_path, "a") as log:
            log.write("Running: " + " ".join(self.cmd) + "\n")
        print("Running: " + " ".join(self.cmd))
        sys.stdout.flush()
        proc = subprocess.Popen(self.cmd, env=self.env, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, start_new_session=(sys.platform != "win32"))
        try:
            return self._follow(proc)
        except BaseException:  # e.g. KeyboardInterrupt: don't leave NatronRenderer running in its own session
            natronrunner.kill_tree(proc)
            raise

    def _follow(self, proc):
        """Follows proc until it exits, or until it stalls: then takes the backtraces and kills it."""
        reader = threading.Thread(target=self._read, args=(proc.stdout,), daemon=True)
        reader.start()
        files = self._files()
        self.last_progress = time.time()
        backtrace = ""
        stalled = False
        while proc.poll() is None:
            time.sleep(1.)
            current = self._files()
            with self.lock:
                if current != files:
                    files = current
                    self._progress()
                idle = time.time() - self.last_progress
            if idle > self.stall_timeout:
                stalled = True
                print("SOAK STALL: no progress for %.0f s, taking the backtraces of all threads" % idle)
                sys.stdout.flush()
                backtrace = dump_backtraces(proc, self.wrapped)
                break
        if stalled:
            natronrunner.kill_tree(proc)
        status = proc.wait()
        reader.join(timeout=30)
        if stalled and self.wrapped:
            # the debugger wrapper printed the backtraces in its log, with the output of NatronRenderer
            with self.lock:
                backtrace += "\n".join(self.lines)
        with self.lock:
            self.longest_gap = max(self.longest_gap, time.time() - self.last_progress)
        return (None if stalled else status), stalled, backtrace


def renderer_pids(proc):
    """The pids of the NatronRenderer processes started by proc (itself, or below a wrapper)."""
    if sys.platform == "win32":
        return [proc.pid]
    pids = [proc.pid] + natronrunner.descendants(proc.pid)
    ret = []
    for pid in pids:
        try:
            comm = subprocess.check_output(["ps", "-o", "comm=", "-p", str(pid)]).decode().strip()
        except (OSError, subprocess.CalledProcessError):
            continue
        if "NatronRenderer" in os.path.basename(comm):
            ret.append(pid)
    return ret or [proc.pid]


def debugger_command(pid):
    """A command that prints the backtrace of every thread of pid, or None if no debugger is found."""
    if sys.platform == "darwin" and shutil.which("lldb"):
        return ["lldb", "--batch", "-p", str(pid), "-o", "thread backtrace all", "-o", "detach"]
    gdb = os.environ.get("SOAK_GDB") or shutil.which("gdb")
    if gdb:
        cmd = [gdb, "-p", str(pid), "-batch", "-ex", "set pagination off", "-ex", "info threads",
               "-ex", "thread apply all bt"]
        # Linux: attaching to a process that is not a child needs ptrace_scope 0, or root
        if sys.platform.startswith("linux") and os.geteuid() != 0 and shutil.which("sudo"):
            try:
                with open("/proc/sys/kernel/yama/ptrace_scope") as f:
                    if f.read().strip() != "0":
                        cmd = ["sudo", "-n"] + cmd
            except OSError:
                pass
        return cmd
    if shutil.which("lldb"):
        return ["lldb", "--batch", "-p", str(pid), "-o", "thread backtrace all", "-o", "detach"]
    return None


def dump_backtraces(proc, wrapped):
    """Prints and returns the backtrace of every thread of the stalled NatronRenderer."""
    text = []
    for pid in renderer_pids(proc):
        if wrapped and sys.platform != "win32":
            # under natron-lldb.sh (or another debugger), a second debugger can't attach: SIGABRT makes lldb stop,
            # print the backtraces ("thread backtrace all") and kill NatronRenderer; the wrapper prints its log.
            print("sending SIGABRT to %d (debugger wrapper)" % pid)
            try:
                os.kill(pid, signal.SIGABRT)
            except OSError as e:
                text.append("kill -ABRT %d: %s" % (pid, e))
            try:
                proc.wait(timeout=DEBUGGER_TIMEOUT)
            except subprocess.TimeoutExpired:
                text.append("the wrapper did not exit %d s after SIGABRT" % DEBUGGER_TIMEOUT)
            continue
        cmd = debugger_command(pid)
        if cmd is None:
            text.append("no debugger (lldb or gdb) to take the backtraces of %d" % pid)
            continue
        print("Running: " + " ".join(cmd))
        try:
            out = subprocess.run(cmd, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                 timeout=DEBUGGER_TIMEOUT, check=False).stdout.decode(errors="replace")
        except (OSError, subprocess.TimeoutExpired) as e:
            out = "%s: %s" % (" ".join(cmd), e)
        text.append(out)
    ret = "\n".join(text)
    sys.stdout.write(ret + "\n")
    sys.stdout.flush()
    return ret


def frame_path(out_dir, pattern, frame):
    """The file of frame for pattern: "png_####.png" (zero-padded to the number of #) or "output#.jpg"."""
    hashes = pattern.count("#")
    return os.path.join(out_dir, pattern.replace("#" * hashes, "%0*d" % (hashes, frame)))


def frames_written(out_dir, pattern, first, last):
    """The frames first..last of pattern (see frame_path()) that exist and are not empty; a video is one file."""
    if "#" not in pattern:
        path = os.path.join(out_dir, pattern)
        return list(range(first, last + 1)) if os.path.isfile(path) and os.path.getsize(path) > 0 else []
    paths = [(frame, frame_path(out_dir, pattern, frame)) for frame in range(first, last + 1)]
    return [frame for frame, path in paths if os.path.isfile(path) and os.path.getsize(path) > 0]


def probe_misses(stats_path, probe):
    """The cache misses of the nodes whose name starts with probe, in a render statistics file (NatronRenderer -s)."""
    misses = 0
    node = None
    try:
        with open(stats_path, errors="replace") as f:
            for line in f:
                if line.startswith("------------------------------- "):
                    node = line.split()[1].rstrip("-")
                elif line.startswith("Nb cache miss: ") and node and node.startswith(probe):
                    misses += int(line.split(":")[1])
    except (OSError, ValueError):
        pass
    return misses


def unrendered_frames(out_dir, writers, first, last, probe):
    """Returns (frames with no render statistics, frames whose statistics show no cache miss of probe).

    The statistics of every image-sequence Write node are read. A frame whose frame-varying probe node never missed
    the cache was not rendered: its images came from a cache.
    """
    sequences = [pattern for pattern in writers.values() if "#" in pattern]
    no_stats, cached = [], []
    for frame in range(first, last + 1):
        stats = [os.path.splitext(frame_path(out_dir, pattern, frame))[0] + "-stats.txt" for pattern in sequences]
        stats = [path for path in stats if os.path.isfile(path)]
        if sequences and not stats:
            no_stats.append(frame)
        elif sequences and not any(probe_misses(path, probe) for path in stats):
            cached.append(frame)
    return no_stats, cached


def compare_with_reference(idiff, reference, output):
    """None if output matches reference (idiff with the BayMax options of Natron-Tests), else the idiff output."""
    result = subprocess.run([idiff, reference, output] + BAYMAX_IDIFF_OPTS, stdin=subprocess.DEVNULL,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=300, check=False)
    out = result.stdout.decode(errors="replace")
    return out if "FAILURE" in out or result.returncode not in (0, 1) else None


def reported_frames(lines, writer):
    """The frames that NatronRenderer reported for writer ("<writer> ==> Frame: N")."""
    tag = writer + " ==> Frame: "
    ret = set()
    for line in lines:
        if tag in line:
            try:
                ret.add(int(line.split(tag, 1)[1].split(",", 1)[0]))
            except ValueError:
                pass
    return ret


class SetupError(Exception):
    """The project of a graph could not be saved."""


def setup_graph(renderer, wrapper, work, graph, opts, timeout):
    """Saves the project of graph; returns (project path, {writer: file pattern}), or None to skip the graph.

    The graph is skipped when render_soak_graph.py skips it (exit status 77: a plug-in it needs is missing). Any
    other failure (a crash, a timeout, no Write node) raises SetupError.
    """
    project = os.path.join(work, graph + ".ntp")
    out_dir = os.path.join(work, "setup-" + graph)
    os.makedirs(out_dir, exist_ok=True)
    env = natronrunner.natron_env(work, SOAK_OUT=out_dir, SOAK_GRAPH=graph, SOAK_SIZE=opts.size, SOAK_SAVE=project,
                                  SOAK_INPUT=os.path.join(work, "input"))
    status, out = natronrunner.run(wrapper + [renderer, "--no-settings", "-t", GRAPH_SCRIPT], env, timeout)
    sys.stdout.write(out)
    if status == natronrunner.SKIP:
        print("graph %s skipped" % graph)
        return None
    writers = {}
    for line in out.splitlines():
        if line.startswith("SOAK writer "):
            name, pattern = line.split()[2:4]
            writers[name] = pattern
    if status != 0 or not writers or not os.path.isfile(project):
        raise SetupError("could not save the project of the %s graph (exit status %s, %d Write nodes)"
                         % (graph, status, len(writers)))
    return project, writers


def setup_baymax(work, natron_tests):
    """Copies BayMax from a Natron-Tests checkout into work; returns its configurations, or None to skip it.

    Each configuration is a dict: conf (its file name), project, first, last, node (the output node), quality, and
    pattern (the output files).
    """
    source = os.path.join(natron_tests, "BayMax") if natron_tests else ""
    if not os.path.isdir(os.path.join(source, "Robot")):
        print("graph baymax skipped: no BayMax/Robot in the Natron-Tests checkout %r (--natron-tests; runTests.sh "
              "downloads Robot)" % natron_tests)
        return None
    target = os.path.join(work, "BayMax")
    if not os.path.isdir(target):
        shutil.copytree(source, target, ignore=shutil.ignore_patterns("output*", "comp*", "*.lock", "*.autosave"))
    confs = []
    for conf in BAYMAX_CONFS:
        with open(os.path.join(target, conf)) as f:
            lines = [line.strip() for line in f]
        first, last = (int(v) for v in lines[1].split())
        confs.append({"conf": conf, "project": os.path.join(target, lines[0]), "first": first, "last": last,
                      "node": lines[2], "quality": lines[4] if len(lines) > 4 and lines[4] else "85",
                      "pattern": "%s_output#.%s" % (conf, lines[3])})
    return confs


def write_input_sequence(directory, count, width, height):
    """Writes the image sequence that the Read node of the writers graph reads (in_0001.png ...)."""
    os.makedirs(directory, exist_ok=True)
    for frame in range(1, count + 1):
        # the size changes a little, so that each frame is a different image
        natronrunner.write_png(os.path.join(directory, "in_%04d.png" % frame), width - frame % 8, height)


def one_run(index, renderer, wrapper, work, graph, setup, driver, config, opts, with_stats):
    """Runs one soak run; returns its summary (a dict).

    with_stats: the cli runs use render statistics (-s) to check that each frame was rendered; every other cli run
    of each graph doesn't, so that the command line that users run is soaked too.
    """
    if graph == "baymax":
        writers = {c["conf"]: c["pattern"] for c in setup}
        first, last = min(c["first"] for c in setup), max(c["last"] for c in setup)
    else:
        project, writers = setup
        first, last = 1, opts.frames
    run_dir = os.path.join(work, "run%03d" % index)
    out_dir = os.path.join(run_dir, "out")
    os.makedirs(out_dir)
    log = os.path.join(run_dir, "log.txt")
    settings = []
    for s in CONFIGS[config]:
        settings += ["--setting", s]
    # Each run has its own disk cache: a stall must not depend on the images an earlier run left. It starts with a
    # copy of the cache of the setup runs, which has the OFX plug-in cache: NatronRenderer doesn't scan the plug-ins
    # again, which may take longer than the stall timeout on a slow machine.
    cache = os.path.join(run_dir, "cache")
    if os.path.isdir(os.path.join(work, "cache")):
        shutil.copytree(os.path.join(work, "cache"), cache)
    else:  # BayMax alone: no setup run
        os.makedirs(cache)
    # A gain that changes from run to run (Grade nodes after the generators and the Read nodes): no image of this run
    # can be one of an earlier run. Each run is also a new NatronRenderer, with a RAM cache of its own.
    gain = 1 + (index % 10 + 1) * 1e-4
    stats = ["-s"] if with_stats and driver == "cli" else []
    base_env = dict(SOAK_OUT=out_dir, SOAK_GRAPH=graph, SOAK_SIZE=opts.size, SOAK_INPUT=os.path.join(work, "input"),
                    NATRON_DISK_CACHE_PATH=cache, SOAK_RUN_GAIN=repr(gain))
    summary = {"run": index, "graph": graph, "driver": driver, "config": config, "frames": last - first + 1,
               # BayMax: its configurations render frames first..last once in all
               "writers": 1 if graph == "baymax" else len(writers), "stats": bool(stats), "result": "ok",
               "seconds": 0., "longest_gap": 0., "details": ""}
    frame_range = "%d-%d" % (first, last)
    steps = []
    if graph == "baymax":
        # each configuration of Natron-Tests, as runTests.sh renders it, with render statistics (-s)
        for c in setup:
            # the BayMax directory is on NATRON_PLUGIN_PATH, as in runTests.sh
            plugin_path = os.pathsep.join(p for p in (os.path.dirname(c["project"]),
                                                      os.environ.get("NATRON_PLUGIN_PATH", "")) if p)
            env = dict(base_env, SOAK_OUTPUT_NODE=c["node"], SOAK_OUTPUT_FILE=os.path.join(out_dir, c["pattern"]),
                       SOAK_FIRST=str(c["first"]), SOAK_LAST=str(c["last"]), SOAK_QUALITY=c["quality"],
                       NATRON_PLUGIN_PATH=plugin_path)
            steps.append((wrapper + [renderer, "--no-settings"] + settings +
                          stats + ["-w", "SoakWrite", "-l", BAYMAX_ONLOAD, c["project"]], env, [0]))
    elif driver == "cli":
        cmd = wrapper + [renderer, "--no-settings"] + settings + stats + ["-l", GAIN_ONLOAD]
        for name, pattern in writers.items():
            # the project was saved with the output directory of setup_graph()
            cmd += ["-w", name, os.path.join(out_dir, pattern)]
        steps.append((cmd + [project, frame_range], dict(base_env), [0]))
    else:
        # the last Write node is aborted halfway, then rendered again by a second NatronRenderer
        abort_frame = first + (last - first) // 2 if len(writers) > 1 else 0
        cmd = wrapper + [renderer, "--no-settings"] + settings + ["-t", GRAPH_SCRIPT]
        env = dict(base_env, SOAK_RENDER=frame_range, SOAK_ABORT_FRAME=str(abort_frame))
        steps.append((cmd, env, [ABORT_STATUS] if abort_frame else [0]))
        if abort_frame:
            steps.append((cmd, dict(base_env, SOAK_RENDER=frame_range, SOAK_WRITERS=list(writers)[-1]), [0]))
    start = time.time()
    last_lines = []  # the output of the last step, which wrote the final version of each file
    for cmd, env, statuses in steps:
        watchdog = Watchdog(cmd, natronrunner.natron_env(run_dir, **env), out_dir, opts.stall_timeout, log,
                            bool(wrapper))
        status, stalled, backtrace = watchdog.run()
        last_lines = watchdog.output_lines()
        summary["longest_gap"] = round(max(summary["longest_gap"], watchdog.longest_gap), 1)
        if stalled:
            summary["result"] = "STALL"
            with open(os.path.join(run_dir, "backtrace.txt"), "w") as f:
                f.write(backtrace)
            summary["details"] = "no progress for %g s; backtraces in %s" % (opts.stall_timeout, run_dir)
            break
        older_natron = "SOAK no natron.quit(): no abort" in last_lines  # it can't abort: exit status 0
        if status not in statuses and not (older_natron and status == 0):
            summary["result"] = "FAIL"
            summary["details"] = "exit status %s, expected %s; log: %s" % (status, statuses, log)
            sys.stdout.write("\n".join(last_lines[-40:]) + "\n")
            break
    summary["seconds"] = round(time.time() - start, 1)
    if summary["result"] == "ok":
        missing = []
        for name, pattern in writers.items():
            if graph == "baymax":
                conf = [c for c in setup if c["conf"] == name][0]
                expected = conf["last"] - conf["first"] + 1
                done = frames_written(out_dir, pattern, conf["first"], conf["last"])
            else:
                expected = last - first + 1
                done = frames_written(out_dir, pattern, first, last)
            if len(done) != expected:
                missing.append("%s: %d of %d frames" % (name, len(done), expected))
            elif "#" not in pattern:
                # a video is one file: each frame must have been reported, by one NatronRenderer (each one writes the
                # whole file again; the restart step renders it in one render)
                reported = reported_frames(last_lines, name)
                if len(reported) != last - first + 1:
                    missing.append("%s: %d of %d frames reported" % (name, len(reported), last - first + 1))
        if stats and not missing:
            parts = ([({c["conf"]: c["pattern"]}, c["first"], c["last"]) for c in setup] if graph == "baymax"
                     else [(writers, first, last)])
            no_stats, cached = [], []
            for part_writers, part_first, part_last in parts:
                part_no_stats, part_cached = unrendered_frames(out_dir, part_writers, part_first, part_last,
                                                               PROBES[graph])
                no_stats += part_no_stats
                cached += part_cached
            if no_stats:
                missing.append("frames %s have no render statistics" % ",".join(str(f) for f in no_stats[:20]))
            if cached:
                missing.append("frames %s not rendered (no cache miss of %s in their render statistics)"
                               % (",".join(str(f) for f in cached[:20]), PROBES[graph]))
        if graph == "baymax" and not missing and opts.idiff_path:
            # the first frame of each configuration, against the reference of Natron-Tests
            for c in setup:
                reference = os.path.join(work, "BayMax", "reference%d.jpg" % c["first"])
                diff = compare_with_reference(opts.idiff_path, reference, frame_path(out_dir, c["pattern"], c["first"]))
                if diff:
                    missing.append("%s frame %d differs from the reference: %s" % (c["conf"], c["first"], diff[-300:]))
        if missing:
            summary["result"] = "FAIL"
            summary["details"] = "; ".join(missing)
    # the images are checked: remove them, so that thousands of frames over many runs don't fill the disk
    shutil.rmtree(out_dir, ignore_errors=True)
    shutil.rmtree(cache, ignore_errors=True)
    print("SOAK run %(run)d %(graph)s %(driver)s %(config)s: %(result)s, %(frames)d frames x %(writers)d writers, "
          "%(seconds)g s, longest gap %(longest_gap)g s %(details)s" % summary)
    sys.stdout.flush()
    return summary


def run_soak(renderer, wrapper, work, opts):
    """Sets the graphs up and runs the soak runs; returns the exit status."""
    width, height = (int(v) for v in opts.size.split("x"))
    write_input_sequence(os.path.join(work, "input"), INPUT_FRAMES, width, height)
    setups = {}
    for graph in opts.graphs.split(","):
        try:
            if graph == "baymax":
                setup = setup_baymax(work, opts.natron_tests)
            else:
                setup = setup_graph(renderer, wrapper, work, graph, opts, 600)
        except SetupError as e:
            print("FAILED: %s" % e)
            return 1
        if setup is not None:
            setups[graph] = setup
    if not setups:
        print("SKIP: no graph could be built (set OFX_PLUGIN_PATH to openfx-misc and openfx-io)")
        return natronrunner.SKIP
    # BayMax is a project: rendered from the command line only
    combos = [(g, d) for g, d in itertools.product(sorted(setups), opts.drivers.split(","))
              if g != "baymax" or d == "cli"]
    if "baymax" in setups and ("baymax", "cli") not in combos:
        combos.append(("baymax", "cli"))
    configs = opts.configs.split(",")
    runs = opts.runs or len(combos)
    deadline = time.time() + opts.duration if opts.duration else None
    summaries = []
    while True:
        index = len(summaries)
        more = index < runs if deadline is None else time.time() < deadline
        if not more:
            break
        graph, driver = combos[index % len(combos)]
        config = configs[index % len(configs)]
        with_stats = (index // len(combos)) % 2 == 0  # every other run of each (graph, driver)
        summaries.append(one_run(index, renderer, wrapper, work, graph, setups[graph], driver, config, opts,
                                 with_stats))
    with open(os.path.join(work, "summary.json"), "w") as f:
        json.dump(summaries, f, indent=1)
    failed = [s for s in summaries if s["result"] != "ok"]
    frames = sum(s["frames"] * s["writers"] for s in summaries if s["result"] == "ok")
    print("SOAK summary: %d runs, %d frames written by the runs that passed, %d failed (%d stalls), %.0f s, "
          "longest gap %g s"
          % (len(summaries), frames, len(failed), sum(s["result"] == "STALL" for s in failed),
             sum(s["seconds"] for s in summaries), max(s["longest_gap"] for s in summaries)))
    for s in failed:
        print("  run %(run)d %(graph)s %(driver)s %(config)s: %(result)s %(details)s" % s)
    return 1 if failed else 0


def main(argv):
    opts = parse_args(argv[1:])
    wrapper = natronrunner.wrapper(["--debugger"] if opts.debugger else [])
    for option, variable in ((opts.ofx_path, "OFX_PLUGIN_PATH"), (opts.pyplug_path, "NATRON_PLUGIN_PATH")):
        if option:
            paths = [p for p in os.environ.get(variable, "").split(os.pathsep) if p]
            os.environ[variable] = os.pathsep.join([option] + paths)
    if not os.path.isfile(os.environ.get("OCIO", "")):
        print("SKIP: the writers of openfx-io need an OpenColorIO configuration: set OCIO")
        return natronrunner.SKIP
    opts.idiff_path = opts.idiff or shutil.which("idiff") or ""
    work = opts.out or tempfile.mkdtemp(prefix="natron-soak-")
    os.makedirs(work, exist_ok=True)
    try:
        return run_soak(renderer=opts.renderer, wrapper=wrapper, work=work, opts=opts)
    finally:
        if not opts.out:
            shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
