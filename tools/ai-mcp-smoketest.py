#!/usr/bin/env python3
"""Smoke-test Natron's AIMcpServer without involving a model.

Speaks MCP (JSON-RPC 2.0 over HTTP) directly at a running Natron that has had the
AI Assistant panel opened at least once, which is what starts the server.

    python tools/ai-mcp-smoketest.py http://127.0.0.1:PORT/mcp TOKEN

Exercises, in order: the bearer-token check, `initialize`, `tools/list`, and the
two read-only tools. That covers the transport, the auth gate, the hop onto the
GUI thread and the tool dispatch -- everything except the agent CLI.

Read-only by default. Pass --mutate to additionally create a node, set a
parameter on it and delete it again; with the panel open you should see one
approval dialog for the delete, and the whole sequence should collapse into a
single Ctrl+Z. --mutate needs a project open in Natron.

The default --plugin is a built-in Dot, which exists even with no OpenFX
plug-ins installed. A stock source build has no OFX plug-ins at all, so
net.sf.openfx.GradePlugin and everything else from openfx-misc will report
PLUGIN_NOT_FOUND until those bundles are on OFX_PLUGIN_PATH.

Only the Python standard library is used, so this runs with any Python 3.
"""

import argparse
import json
import sys
import urllib.error
import urllib.request

_next_id = 0


def rpc(url, token, method, params=None, expect_error=False):
    """Sends one JSON-RPC request and returns the parsed response."""
    global _next_id
    _next_id += 1

    body = {"jsonrpc": "2.0", "id": _next_id, "method": method}
    if params is not None:
        body["params"] = params

    request = urllib.request.Request(
        url,
        data=json.dumps(body).encode("utf-8"),
        headers={
            "Content-Type": "application/json",
            "Authorization": "Bearer " + token,
        },
        method="POST",
    )

    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            raw = response.read().decode("utf-8")
    except urllib.error.HTTPError as e:
        if expect_error:
            return {"_http_status": e.code}
        raise

    if not raw.strip():
        return {}

    return json.loads(raw)


def call_tool(url, token, name, arguments=None):
    """Calls one MCP tool and unwraps its result.

    Returns (ok, payload). A domain failure arrives as isError:true with the
    structured code/message/hint object -- that is a successful round-trip, not a
    transport failure, so it is reported rather than raised.
    """
    response = rpc(url, token, "tools/call",
                   {"name": name, "arguments": arguments or {}})

    if "error" in response:
        return False, response["error"]

    result = response.get("result", {})
    is_error = result.get("isError", False)

    payload = result.get("structuredContent")
    if payload is None:
        content = result.get("content", [])
        if content and content[0].get("type") == "text":
            try:
                payload = json.loads(content[0]["text"])
            except ValueError:
                payload = content[0]["text"]

    return (not is_error), payload


def show(label, ok, payload):
    mark = "ok  " if ok else "FAIL"
    print("[%s] %s" % (mark, label))
    print(json.dumps(payload, indent=2)[:1200])
    print()

    return ok


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("url", help="e.g. http://127.0.0.1:52341/mcp")
    parser.add_argument("token", help="bearer token from the AI Assistant panel")
    parser.add_argument("--mutate", action="store_true",
                        help="also create and delete a node (needs a project open)")
    parser.add_argument("--plugin", default="fr.inria.built-in.Dot",
                        help="plugin to create with --mutate. The default is a "
                             "built-in node, so it works even with no OpenFX "
                             "plug-ins installed. net.sf.openfx.GradePlugin and "
                             "friends require openfx-misc to be present.")
    args = parser.parse_args()

    failures = 0

    # 1. The auth gate must reject a wrong token before anything else runs.
    print("[....] rejecting a bad token")
    bad = rpc(args.url, args.token + "-wrong", "initialize", {},
              expect_error=True)
    if bad.get("_http_status") == 401:
        print("[ok  ] bad token rejected with 401\n")
    else:
        print("[FAIL] a bad token was NOT rejected: %r\n" % (bad,))
        failures += 1

    # 2. Handshake.
    response = rpc(args.url, args.token, "initialize",
                   {"protocolVersion": "2025-06-18",
                    "capabilities": {},
                    "clientInfo": {"name": "smoketest", "version": "1"}})
    info = response.get("result", {})
    if not show("initialize", "serverInfo" in info, info):
        failures += 1

    # 3. Catalogue.
    response = rpc(args.url, args.token, "tools/list")
    tools = response.get("result", {}).get("tools", [])
    names = [t.get("name") for t in tools]
    if not show("tools/list -> %d tools" % len(tools), bool(tools), names):
        failures += 1

    # 4. Read-only tools.
    ok, payload = call_tool(args.url, args.token, "natron_status")
    if not show("natron_status", ok, payload):
        failures += 1

    ok, payload = call_tool(args.url, args.token, "graph_list_nodes")
    if not show("graph_list_nodes", ok, payload):
        failures += 1

    # 5. A deliberate failure: the error contract should come back structured,
    #    with a code and a hint, not as a transport error or a stack trace.
    ok, payload = call_tool(args.url, args.token, "param_get",
                            {"node": "NoSuchNode", "param": "mix"})
    good = (not ok) and isinstance(payload, dict) and payload.get("code") == "NODE_NOT_FOUND"
    if not show("param_get on a missing node -> NODE_NOT_FOUND", good, payload):
        failures += 1

    if args.mutate:
        ok, node = call_tool(args.url, args.token, "node_create",
                             {"pluginID": args.plugin, "x": 0, "y": 0})
        if not show("node_create %s" % args.plugin, ok, node):
            failures += 1
        elif isinstance(node, dict) and node.get("scriptName"):
            script_name = node["scriptName"]

            # Only attempt a write if the node actually has that parameter --
            # a Dot has none, a Grade has blackPoint.
            ok, payload = call_tool(args.url, args.token, "param_get",
                                    {"node": script_name, "param": "blackPoint"})
            if ok:
                ok, payload = call_tool(args.url, args.token, "param_set",
                                        {"node": script_name,
                                         "param": "blackPoint",
                                         "value": 0.02,
                                         "dimension": 0})
                if not show("param_set blackPoint=0.02", ok, payload):
                    failures += 1
            else:
                print("[skip] %s has no 'blackPoint'; skipping param_set\n"
                      % args.plugin)

            print(">>> a confirmation dialog should appear in Natron now")
            ok, payload = call_tool(args.url, args.token, "node_delete",
                                    {"node": script_name})
            if not show("node_delete (needs approval)", ok, payload):
                failures += 1

    print("=" * 60)
    if failures:
        print("%d check(s) failed" % failures)

        return 1

    print("all checks passed")

    return 0


if __name__ == "__main__":
    sys.exit(main())
