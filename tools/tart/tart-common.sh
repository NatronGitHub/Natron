# shellcheck shell=bash
# Helpers shared by natron-sdk.sh and natron-build.sh (sourced, not executed).
#
# The VMs use tart's default "shared" (NAT) network: they are reachable from this host
# only, not from the LAN. The cirruslabs images ship with the well-known admin/admin
# account; it is used once, to install a dedicated SSH key, and all later logins use
# that key.
#
# Networking: on some hosts (e.g. with corporate network filters) the guest gets no
# IPv4 DHCP lease and no NAT. The guest is then reached through its IPv6 link-local
# address on the vmnet bridge (found with the tart guest agent), and it gets internet
# access through a reverse SOCKS proxy carried by the SSH connection (ssh -R): the
# proxy listens on the guest's 127.0.0.1:1080 only, and only allows ports 80 and 443.

TART_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
NATRON_SRC=$(cd "$TART_DIR/../.." && pwd)
THIRD_PARTY=$(cd "$NATRON_SRC/.." && pwd)
OSMESA_INSTALL_SRC=${OSMESA_INSTALL_SRC:-$THIRD_PARTY/osmesa-install}

# base image (NATRON_MACOS_BASE_IMAGE in tools/jenkins/macos-release.sh, pinned by digest;
# BASE_IMAGE=...:latest overrides it) and local SDK image names
# shellcheck source=../jenkins/macos-release.sh
. "$NATRON_SRC/tools/jenkins/macos-release.sh"
BASE_IMAGE=${BASE_IMAGE:-$NATRON_MACOS_BASE_IMAGE}
SDK_IMAGE=${SDK_IMAGE:-natron-sdk-sonoma}
VM_CPU=${VM_CPU:-8}
VM_MEMORY=${VM_MEMORY:-16384}   # MB
VM_DISK=${VM_DISK:-250}         # GB (sparse)

TART_KEY=${TART_KEY:-$HOME/.ssh/natron-tart_ed25519}
GUEST_PROXY_PORT=${GUEST_PROXY_PORT:-1080}
GUEST_PROXY=socks5h://127.0.0.1:$GUEST_PROXY_PORT   # as seen from the guest
# destinations the guest may reach through the proxy (OpenSSH >= 8.7)
GUEST_PROXY_ALLOW=${GUEST_PROXY_ALLOW:-"*:80 *:443"}

# per-user runtime files (sockets, address cache, logs): not in the shared /tmp
TART_RUN=${TART_RUN:-${TMPDIR:-/tmp}/natron-tart}
mkdir -p "$TART_RUN" && chmod 700 "$TART_RUN"
# ssh control sockets: a Unix socket path is limited to 104 bytes, and macOS's $TMPDIR
# (/var/folders/xx/.../T/) is too long for "<dir>/<vm>.sock" plus ssh's temporary suffix.
TART_SOCK=${TART_SOCK:-/tmp/natron-tart-$(id -u)}
mkdir -p -m 700 "$TART_SOCK"
if [ -L "$TART_SOCK" ] || [ ! -O "$TART_SOCK" ]; then
    echo "error: $TART_SOCK is not a directory owned by $(id -un)" >&2
    exit 1
fi
chmod 700 "$TART_SOCK"

# cloned VMs share host keys and get new addresses: don't pin host keys (host-only network).
# -F /dev/null: the user's ~/.ssh/config (agent forwarding, ProxyCommand...) never applies to the guest
tart_sshopts() { # vm
    echo "-F /dev/null -i $TART_KEY -o IdentitiesOnly=yes -o ForwardAgent=no -o ForwardX11=no" \
         "-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR" \
         "-o ServerAliveInterval=60 -o ControlPath=$TART_SOCK/$1.sock"
}

# one run at a time: clones share their IPv6 link-local address, and a WIP must not be provisioned twice
tart_lock() {
    mkdir "$TART_RUN/lock" 2>/dev/null ||
        die "another natron-sdk.sh/natron-build.sh run holds $TART_RUN/lock (remove it if that run is gone)"
    TART_LOCKED=1
}
tart_unlock() { [ "${TART_LOCKED:-0}" = 1 ] && rmdir "$TART_RUN/lock" 2>/dev/null; TART_LOCKED=0; }

log() { echo "[$(date '+%F %T')] $*"; }
die() { echo "Error: $*" >&2; exit 1; }

# rsync >= 3 is needed: macOS's /usr/bin/rsync (openrsync) mis-parses IPv6 targets
RSYNC=${RSYNC:-}
if [ -z "$RSYNC" ]; then
    for r in /opt/local/bin/rsync /opt/homebrew/bin/rsync /usr/local/bin/rsync rsync; do
        v=$("$r" --version 2>/dev/null || true)   # no pipe: SIGPIPE + pipefail would fail the test
        case "${v%%$'\n'*}" in rsync*" version "[3-9]*) RSYNC=$r; break ;; esac
    done
fi
[ -n "$RSYNC" ] || die "rsync >= 3 not found (brew install rsync, or sudo port install rsync)"

vm_list_running() { tart list --format json | python3 -c 'import json,sys; print("\n".join(v["Name"] for v in json.load(sys.stdin) if v.get("Running")))'; }
vm_exists()  { tart list --format json | python3 -c 'import json,sys; sys.exit(0 if any(v["Name"]==sys.argv[1] for v in json.load(sys.stdin)) else 1)' "$1"; }
vm_running() { vm_list_running | grep -qx "$1"; }

vm_configure() { # vm (stopped)
    vm_running "$1" && return 0
    tart set "$1" --cpu "$VM_CPU" --memory "$VM_MEMORY"
    # disk can only grow
    tart set "$1" --disk-size "$VM_DISK" 2>/dev/null || true
}

# the vmnet bridge carrying tart's shared network (the one with 192.168.64.1), for IPv6 scope
vm_bridge() {
    local b
    for b in $(ifconfig -l); do
        case "$b" in bridge*) ifconfig "$b" 2>/dev/null | grep -q "inet 192\.168\.64\.1 " && { echo "$b"; return; } ;; esac
    done
    echo bridge100
}

vm_addr_cache() { echo "$TART_RUN/$1.addr"; }

vm_find_addr() { # vm : IPv4 if the guest gets a lease, else its IPv6 link-local address
    local i a
    for i in $(seq 90); do   # the first boot after a disk resize can take several minutes
        # an IPv4 address only counts if ssh answers there: on hosts with network filters the
        # guest may get a lease that the host can't reach (seen 2026-11-22)
        if a=$(tart ip "$1" 2>/dev/null) && nc -z -G 3 "$a" 22 2>/dev/null; then echo "$a"; return 0; fi
        # give DHCP a minute before falling back to IPv6
        if [ "$i" -gt 6 ]; then
            a=$(tart exec "$1" /sbin/ifconfig en0 2>/dev/null | awk '/inet6 fe80/{sub(/%.*/,"",$2); print $2; exit}') || a=
            if [ -n "$a" ] && nc -z -G 3 "$a%$(vm_bridge)" 22 2>/dev/null; then echo "$a%$(vm_bridge)"; return 0; fi
        fi
        sleep 10
    done
    echo "no address for $1 where ssh answers, after 90 attempts (up to about 25 minutes)" >&2
    return 1
}

vm_addr() { # vm : address found by vm_start
    local f
    f=$(vm_addr_cache "$1")
    [ -s "$f" ] || vm_find_addr "$1" > "$f"
    cat "$f"
}

vm_ssh() { # vm cmd...
    local vm=$1; shift
    # shellcheck disable=SC2046
    ssh $(tart_sshopts "$vm") admin@"$(vm_addr "$vm")" "$@"
}

vm_rsync() { # vm src guest-dst [rsync options...]
    local vm=$1 src=$2 dst=$3 addr; shift 3
    addr=$(vm_addr "$vm")
    "$RSYNC" -a -e "ssh $(tart_sshopts "$vm")" "$@" "$src" "admin@[$addr]:$dst"
}

vm_rsync_back() { # vm guest-src dst [rsync options...]
    local vm=$1 src=$2 dst=$3 addr; shift 3
    addr=$(vm_addr "$vm")
    "$RSYNC" -a -e "ssh $(tart_sshopts "$vm")" "$@" "admin@[$addr]:$src" "$dst"
}

vm_start() { # vm : boot (if needed), install the SSH key, open the SSH master carrying the guest's proxy
    local vm=$1 addr others ok=0 sock token
    # clones of an image share their IPv6 link-local address: never run two of our VMs at once
    others=$(vm_list_running | grep -E '^natron-(sdk|build|test)-' | grep -vx "$vm" || true)
    [ -z "$others" ] || die "other Natron VMs are running ($others): stop them first (tart stop <name>)"
    [ -f "$TART_KEY" ] || ssh-keygen -q -t ed25519 -N "" -C "natron-tart" -f "$TART_KEY"
    if ! vm_running "$vm"; then
        log "starting $vm"
        nohup tart run --no-graphics "$vm" > "$TART_RUN/$vm.log" 2>&1 &
    fi
    vm_find_addr "$vm" > "$(vm_addr_cache "$vm")"
    addr=$(vm_addr "$vm")
    for _ in $(seq 60); do
        if ssh $(tart_sshopts "$vm") -o BatchMode=yes -o ConnectTimeout=5 -o ControlPath=none admin@"$addr" true 2>/dev/null; then
            ok=1; break
        fi
        command -v sshpass > /dev/null || die "sshpass is needed once to install the SSH key (brew install hudochenkov/sshpass/sshpass)"
        if sshpass -p admin ssh -F /dev/null -o ForwardAgent=no -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR \
               -o ConnectTimeout=5 -o PubkeyAuthentication=no admin@"$addr" \
               "mkdir -p ~/.ssh && chmod 700 ~/.ssh && cat >> ~/.ssh/authorized_keys && chmod 600 ~/.ssh/authorized_keys" \
               < "$TART_KEY.pub" 2>/dev/null; then
            log "installed SSH key in $vm"
        fi
        sleep 5
    done
    [ "$ok" = 1 ] || die "cannot log into $vm at $addr"
    sock=$TART_SOCK/$vm.sock
    if ! ssh $(tart_sshopts "$vm") -O check admin@"$addr" 2>/dev/null; then
        rm -f "$sock"   # stale socket of a killed master
        ssh $(tart_sshopts "$vm") -fN -M -o ExitOnForwardFailure=yes \
            -o PermitRemoteOpen="$GUEST_PROXY_ALLOW" -R "127.0.0.1:$GUEST_PROXY_PORT" admin@"$addr"
    fi
    # make sure ssh reached this VM (and not another one with the same address)
    token=$(uuidgen)
    tart exec "$vm" /bin/sh -c "echo $token > /tmp/natron-tart-id"
    [ "$(vm_ssh "$vm" cat /tmp/natron-tart-id)" = "$token" ] || die "ssh to $addr does not reach $vm"
    vm_ssh "$vm" "sudo -n true && curl -sS -m 20 -o /dev/null -x $GUEST_PROXY https://github.com" ||
        die "$vm: no sudo or no internet through the proxy"
    log "$vm is up at $addr (internet via $GUEST_PROXY in the guest)"
}

vm_stop() { # vm
    ssh $(tart_sshopts "$1") -O exit admin@dummy 2>/dev/null || true
    if vm_running "$1"; then
        log "stopping $1"
        # by name (tart exec), not by address: never shut down another VM with the same address
        tart exec "$1" sudo -n /sbin/shutdown -h now > /dev/null 2>&1 || true
        for _ in $(seq 60); do vm_running "$1" || break; sleep 5; done
        vm_running "$1" && tart stop "$1"
    fi
    rm -f "$(vm_addr_cache "$1")"
}

vm_sync() { # vm : copy tools/ of the Natron checkout (tracked files, with local changes) and osmesa-install
    local vm=$1
    log "syncing sources into $vm"
    # tools/ in the VM must be an exact copy of the checkout. A finished image has no copy at all
    # (provision.sh's cleanup step deletes it), but a -wip VM resumed by `natron-sdk.sh create`
    # keeps the previous one, and a file deleted from the checkout since then (e.g. a port removed
    # from the MacPorts overlay) would survive there and be published again by tools/MacPorts/sync.sh.
    # --files-from below can't delete, so start from an empty tools/. Nothing else lives there
    # (provision.sh logs go to ~/provision-logs).
    vm_ssh "$vm" "mkdir -p Documents/third_party/Natron && rm -rf Documents/third_party/Natron/tools"
    # only the files git tracks under tools/ (the build checks out Natron itself from GitHub):
    # untracked notes or credentials in the checkout never end up in the image. Local changes to
    # tracked files are copied; to include a new file, `git add -N` it. A tracked file deleted in
    # the working tree but not yet committed is skipped (--ignore-missing-args).
    (cd "$NATRON_SRC" && git ls-files -z tools) |
        vm_rsync "$vm" "$NATRON_SRC/" Documents/third_party/Natron/ --from0 --files-from=- --ignore-missing-args
    if [ -d "$OSMESA_INSTALL_SRC" ]; then
        (cd "$OSMESA_INSTALL_SRC" && git ls-files -z -c -- . ':!CLAUDE.md') |
            vm_rsync "$vm" "$OSMESA_INSTALL_SRC/" Documents/third_party/osmesa-install/ --from0 --files-from=- --ignore-missing-args
    else
        vm_ssh "$vm" "[ -d Documents/third_party/osmesa-install ] || ALL_PROXY=$GUEST_PROXY git clone https://github.com/devernay/osmesa-install.git Documents/third_party/osmesa-install"
    fi
}

vm_provision() { # vm [VAR=value...] -- steps...  (env passed to provision.sh; PROXY is always set)
    local vm=$1; shift
    local envs=("PROXY=$GUEST_PROXY")
    while [ $# -gt 0 ] && [ "$1" != "--" ]; do envs+=("$(printf '%q' "$1")"); shift; done
    [ "${1:-}" = "--" ] && shift
    vm_ssh "$vm" "env ${envs[*]} bash Documents/third_party/Natron/tools/tart/provision.sh $*"
}
