#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
    printf 'usage: %s <renode-binary> <renode-script>\n' "$0" >&2
    exit 2
fi

renode_binary=$1
renode_script=$2
gdb_host=127.0.0.1
gdb_port=3333
renode_pid=

cleanup() {
    if [[ -n "${renode_pid}" ]] && kill -0 "${renode_pid}" 2>/dev/null; then
        kill "${renode_pid}" 2>/dev/null || true
        wait "${renode_pid}" 2>/dev/null || true
    fi
}

trap cleanup EXIT INT TERM

"${renode_binary}" "${renode_script}" &
renode_pid=$!

printf 'RENODE_GDB_WAITING\n'

for _ in {1..100}; do
    if ! kill -0 "${renode_pid}" 2>/dev/null; then
        wait "${renode_pid}"
        exit $?
    fi

    if timeout 1 bash -c "</dev/tcp/${gdb_host}/${gdb_port}" 2>/dev/null; then
        printf 'RENODE_GDB_READY\n'
        wait "${renode_pid}"
        exit $?
    fi

    sleep 0.1
done

printf 'Timed out waiting for Renode GDB server on %s:%s\n' "${gdb_host}" "${gdb_port}" >&2
exit 1
