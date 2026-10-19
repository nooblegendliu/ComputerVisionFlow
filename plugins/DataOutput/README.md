# DataOutput adaptation

Source: FlowCV `b09c48e`, Plugins/DataOutput (GPL-3.0). The five exported names, ports, default settings, JSON state keys and wire representations are preserved. The targets are CsvFile, UdpSend, TcpSend, OscSend and SerialSend; they are loaded through the existing `.fp` / `Create` mechanism. Optional QWidget Controls are compiled only with `VISIONFLOW_BUILD_STUDIO`.

Dependencies are the original asio2 commit `d18ad5426c9ba23dde06a6177619d0196c5993ee` (`include/` and bundled standalone ASIO), and FlowCV's oscpack sources. Licenses remain alongside the sources; standalone ASIO uses the included Boost Software License. Serial enumeration retains the original platform implementations.

## Preserved behavior

- UDP/TCP/Serial use the first available input in bool → int → float → str → json order. A false bool still takes precedence. The original string input sends **only its first character**, including a zero byte for an empty string. Numeric binary values use the host's native representation; text uses `std::to_string`. EOL is appended to non-JSON payloads and JSON text, never to BSON/CBOR/MessagePack/UBJSON. Rate settings are the original selection indices, not literal Hz.
- TCP retains one-second automatic reconnect. OSC strings form address suffixes, and JSON produces an immediate bundle of `/ref_frame` and `/data[/data_type]/field/index` messages, with strings appended to addresses. Nested object fields remain unexpanded.
- CSV retains `start`/`frame`/`json` inputs, the original commented header, trailing commas, one row per `data` entry, and duplicate frame suppression. Like upstream, the computation does not check the node enable flag. Rotating file indices retain the original inclusive `0..num_files` wrap. The format does not introduce general-purpose string quoting or nested object columns.
- Runtime Controls follow the original resource-change triggers separately from `SetState` project restoration. Changing CSV `Max Size KB` preserves the open file, accumulated rows, counters and duplicate-frame suppression; path/header/limit/rotation edits still start a new file. Sender rate, encoding, EOL, binary and OSC-prefix edits preserve the existing connection. Network endpoint edits reconnect; serial port and framing edits reopen the selected port. Loading state still restores resources and retains the original JSON schema.

## Necessary fixes

- Replaced UDP/OSC async calls that referenced destroyed stack buffers and never pumped their io_service with completed datagram sends. TCP/Serial enqueue owning byte vectors/strings instead of non-owning ASIO buffers. TCP connection setup is asynchronous so an unreachable endpoint does not block the GUI; destructor stops and joins workers.
- Serialized settings, UI access and processing. A weak UI bridge is invalidated before resource destruction; no QWidget enters the computation layer.
- Validate state types/indices before assignment, prevent fixed-buffer overwrites, contain malformed JSON/oversized OSC and I/O failures, and expose runtime connection/file errors in Controls. Compressed IPv6 literals now use ASIO's own parser.
- Corrected the original array-element integer check in CSV/OSC, which tested the whole array and silently dropped integer coordinates. No change to scalar or float formatting.
- CSV uses UTF-8 paths, handles relative rotated paths, resets duplicate-frame suppression when starting a new file, and uses portable millisecond timestamps. Opening after failure clears the stream error state.
- Serial enumeration returns a defined fallback for missing indices, guards Windows registry buffers and uses explicit ANSI SetupAPI calls so optional Qt Unicode defines do not change the original enumeration ABI. Settings changes reopen the selected port, including baud rate changes.

## Verification boundary

`stage5_data_test` covers actual dynamic plugins with isolated CSV files and local-only UDP/TCP/OSC loopback, formatting, invalid-state recovery, reconnect and lifecycle; Qt builds additionally exercise Controls, CSV edits during writing, payload edits on an accepted TCP connection, UDP/OSC sender endpoint continuity, UDP destination edits and serial refresh. Hardware serial transmission requires a real loopback device and is not represented by an enumeration or invalid-port test.
