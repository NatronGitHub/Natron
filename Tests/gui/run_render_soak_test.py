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
after generators, Reformat, expressions, motion blur, Merge, FrameHold, Blur and DiskCache; the "sequential"
graph: a plug-in rendered in frame order, and WriteFFmpeg), driven one of two ways:
- cli: "NatronRenderer -w <Write> <file> ... project.ntp first-last", all the Write nodes rendered at once;
- python: "NatronRenderer -t render_soak_graph.py", which renders with app.render(): one Write node in many
  short renders, the others together, and the last one aborted with natron.quit() from a callback;
  a second NatronRenderer then renders that Write node again (restart).
with one threading configuration (--setting): the runs go through the combinations in turn.

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
  --graphs LIST      comma-separated graphs (default writers,sequential)
  --drivers LIST     comma-separated drivers (default cli,python)
  --configs LIST     comma-separated threading configurations (default: all, see CONFIGS)
  --size WxH         size of the rendered images (default 320x180)
  --out DIR          keep the logs, backtraces and summary in DIR (default: a temporary directory, removed)
  --ofx-path DIR     add DIR to OFX_PLUGIN_PATH (the test plug-ins, for net.natron.test.Sequential)
  --debugger         run NatronRenderer under natron-lldb.sh (or set NATRON_TEST_WRAPPER, see natronrunner.py)

Exit status: 0 on success, 77 if skipped (no OpenColorIO configuration, or the plug-ins of openfx-misc and
openfx-io are not on OFX_PLUGIN_PATH), 1 on failure. A stall prints "SOAK STALL".
"""

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
}


def parse_args(argv):
    opts = {"frames": 120, "runs": 0, "duration": 0., "stall-timeout": 120., "graphs": "writers,sequential",
            "drivers": "cli,python", "configs": ",".join(CONFIGS), "size": "320x180", "out": "", "ofx-path": ""}
    args = list(argv)
    renderer = args.pop(0)
    rest = []
    while args:
        arg = args.pop(0)
        name = arg[2:]
        if arg.startswith("--") and name in opts:
            value = args.pop(0)
            opts[name] = type(opts[name])(value) if not isinstance(opts[name], str) else value
        else:
            rest.append(arg)
    unknown = [c for c in opts["configs"].split(",") if c not in CONFIGS]
    if unknown:
        raise SystemExit("unknown configurations: " + ",".join(unknown))
    return renderer, opts, rest


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
    pids = [proc.pid] + natronrunner._descendants(proc.pid)  # pylint: disable=protected-access
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
    if shutil.which("gdb"):
        cmd = ["gdb", "-p", str(pid), "-batch", "-ex", "set pagination off", "-ex", "info threads",
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
                proc.wait(timeout=120)
            except subprocess.TimeoutExpired:
                text.append("the wrapper did not exit 120 s after SIGABRT")
            continue
        cmd = debugger_command(pid)
        if cmd is None:
            text.append("no debugger (lldb or gdb) to take the backtraces of %d" % pid)
            continue
        print("Running: " + " ".join(cmd))
        try:
            out = subprocess.run(cmd, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                 timeout=600, check=False).stdout.decode(errors="replace")
        except (OSError, subprocess.TimeoutExpired) as e:
            out = "%s: %s" % (" ".join(cmd), e)
        text.append(out)
    ret = "\n".join(text)
    sys.stdout.write(ret + "\n")
    sys.stdout.flush()
    return ret


def frames_written(out_dir, pattern, first, last):
    """The frames first..last of pattern ("png_####.png") that exist and are not empty."""
    if "#" not in pattern:  # a video: one file
        path = os.path.join(out_dir, pattern)
        return list(range(first, last + 1)) if os.path.isfile(path) and os.path.getsize(path) > 0 else []
    prefix, _, suffix = pattern.partition("####")
    ret = []
    for frame in range(first, last + 1):
        path = os.path.join(out_dir, "%s%04d%s" % (prefix, frame, suffix))
        if os.path.isfile(path) and os.path.getsize(path) > 0:
            ret.append(frame)
    return ret


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


def setup_graph(renderer, wrapper, work, graph, size, timeout):
    """Saves the project of graph; returns (project path, {writer: file pattern}), or None to skip."""
    project = os.path.join(work, graph + ".ntp")
    out_dir = os.path.join(work, "setup-" + graph)
    os.makedirs(out_dir, exist_ok=True)
    env = natronrunner.natron_env(work, SOAK_OUT=out_dir, SOAK_GRAPH=graph, SOAK_SIZE=size, SOAK_SAVE=project)
    status, out = natronrunner.run(wrapper + [renderer, "--no-settings", "-t", GRAPH_SCRIPT], env, timeout)
    sys.stdout.write(out)
    writers = {}
    for line in out.splitlines():
        if line.startswith("SOAK writer "):
            name, pattern = line.split()[2:4]
            writers[name] = pattern
    if status == natronrunner.SKIP or not writers:
        print("graph %s skipped" % graph)
        return None
    if status != 0 or not os.path.isfile(project):
        raise RuntimeError("could not save the project of the %s graph (status %s)" % (graph, status))
    return project, writers


def one_run(index, renderer, wrapper, work, graph, setup, driver, config, opts):
    """Runs one soak run; returns its summary (a dict)."""
    project, writers = setup
    first, last = 1, opts["frames"]
    run_dir = os.path.join(work, "run%03d" % index)
    out_dir = os.path.join(run_dir, "out")
    os.makedirs(out_dir)
    log = os.path.join(run_dir, "log.txt")
    settings = []
    for s in CONFIGS[config]:
        settings += ["--setting", s]
    base_env = dict(SOAK_OUT=out_dir, SOAK_GRAPH=graph, SOAK_SIZE=opts["size"],
                    # each run has its own caches: a stall must not depend on what an earlier run left
                    NATRON_DISK_CACHE_PATH=os.path.join(run_dir, "cache"))
    summary = {"run": index, "graph": graph, "driver": driver, "config": config, "frames": last - first + 1,
               "writers": len(writers), "result": "ok", "seconds": 0., "longest_gap": 0., "details": ""}
    steps = []
    if driver == "cli":
        cmd = wrapper + [renderer, "--no-settings"] + settings
        for name, pattern in writers.items():
            # the project was saved with the output directory of setup_graph()
            cmd += ["-w", name, os.path.join(out_dir, pattern)]
        steps.append((cmd + [project, "%d-%d" % (first, last)], dict(base_env), [0]))
    else:
        # the last Write node is aborted halfway, then rendered again by a second NatronRenderer
        abort_frame = first + (last - first) // 2 if len(writers) > 1 else 0
        cmd = wrapper + [renderer, "--no-settings"] + settings + ["-t", GRAPH_SCRIPT]
        env = dict(base_env, SOAK_RENDER="%d-%d" % (first, last), SOAK_ABORT_FRAME=str(abort_frame))
        steps.append((cmd, env, [ABORT_STATUS] if abort_frame else [0]))
        if abort_frame:
            steps.append((cmd, dict(base_env, SOAK_RENDER="%d-%d" % (first, last),
                                    SOAK_WRITERS=list(writers)[-1]), [0]))
    start = time.time()
    lines = []
    for cmd, env, statuses in steps:
        watchdog = Watchdog(cmd, natronrunner.natron_env(run_dir, **env), out_dir, opts["stall-timeout"], log,
                            bool(wrapper))
        status, stalled, backtrace = watchdog.run()
        lines += watchdog.lines
        summary["longest_gap"] = round(max(summary["longest_gap"], watchdog.longest_gap), 1)
        if stalled:
            summary["result"] = "STALL"
            with open(os.path.join(run_dir, "backtrace.txt"), "w") as f:
                f.write(backtrace)
            summary["details"] = "no progress for %g s; backtraces in %s" % (opts["stall-timeout"], run_dir)
            break
        if status == 0 and ABORT_STATUS in statuses and "SOAK no natron.quit(): no abort" in watchdog.lines:
            pass  # an older Natron, without natron.quit()
        elif status not in statuses:
            summary["result"] = "FAIL"
            summary["details"] = "exit status %s, expected %s; log: %s" % (status, statuses, log)
            sys.stdout.write("\n".join(watchdog.lines[-40:]) + "\n")
            break
    summary["seconds"] = round(time.time() - start, 1)
    if summary["result"] == "ok":
        missing = []
        for name, pattern in writers.items():
            done = frames_written(out_dir, pattern, first, last)
            if len(done) != last - first + 1:
                missing.append("%s: %d of %d frames" % (name, len(done), last - first + 1))
            elif "#" not in pattern:
                # a video: every frame must have been reported
                reported = reported_frames(lines, name)
                if reported and len(reported) != last - first + 1:
                    missing.append("%s: %d of %d frames reported" % (name, len(reported), last - first + 1))
        if missing:
            summary["result"] = "FAIL"
            summary["details"] = "missing frames: " + "; ".join(missing)
    # the images are checked: remove them, so that thousands of frames over many runs don't fill the disk
    shutil.rmtree(out_dir, ignore_errors=True)
    shutil.rmtree(os.path.join(run_dir, "cache"), ignore_errors=True)
    print("SOAK run %(run)d %(graph)s %(driver)s %(config)s: %(result)s, %(frames)d frames x %(writers)d writers, "
          "%(seconds)g s, longest gap %(longest_gap)g s %(details)s" % summary)
    sys.stdout.flush()
    return summary


def main(argv):
    if len(argv) < 2:
        sys.stderr.write(__doc__)
        return 2
    renderer, opts, rest = parse_args(argv[1:])
    wrapper = natronrunner.wrapper(rest)
    if opts["ofx-path"]:
        paths = [p for p in os.environ.get("OFX_PLUGIN_PATH", "").split(os.pathsep) if p]
        os.environ["OFX_PLUGIN_PATH"] = os.pathsep.join([opts["ofx-path"]] + paths)
    if not os.path.isfile(os.environ.get("OCIO", "")):
        print("SKIP: the writers of openfx-io need an OpenColorIO configuration: set OCIO")
        return natronrunner.SKIP
    work = opts["out"] or tempfile.mkdtemp(prefix="natron-soak-")
    os.makedirs(work, exist_ok=True)
    try:
        setups = {}
        for graph in opts["graphs"].split(","):
            setup = setup_graph(renderer, wrapper, work, graph, opts["size"], 600)
            if setup is not None:
                setups[graph] = setup
        if not setups:
            print("SKIP: no graph could be built (set OFX_PLUGIN_PATH to openfx-misc and openfx-io)")
            return natronrunner.SKIP
        combos = list(itertools.product(sorted(setups), opts["drivers"].split(",")))
        configs = opts["configs"].split(",")
        runs = opts["runs"] or len(combos)
        deadline = time.time() + opts["duration"] if opts["duration"] else None
        summaries = []
        index = 0
        while (index < runs) if deadline is None else (time.time() < deadline):
            graph, driver = combos[index % len(combos)]
            config = configs[index % len(configs)]
            summaries.append(one_run(index, renderer, wrapper, work, graph, setups[graph], driver, config, opts))
            index += 1
        with open(os.path.join(work, "summary.json"), "w") as f:
            json.dump(summaries, f, indent=1)
        failed = [s for s in summaries if s["result"] != "ok"]
        frames = sum(s["frames"] * s["writers"] for s in summaries)
        print("SOAK summary: %d runs, %d frames written, %d failed (%d stalls), %.0f s, longest gap %g s"
              % (len(summaries), frames, len(failed), sum(s["result"] == "STALL" for s in failed),
                 sum(s["seconds"] for s in summaries), max(s["longest_gap"] for s in summaries)))
        for s in failed:
            print("  run %(run)d %(graph)s %(driver)s %(config)s: %(result)s %(details)s" % s)
        return 1 if failed else 0
    finally:
        if not opts["out"]:
            shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
