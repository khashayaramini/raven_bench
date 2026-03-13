import re
from collections import defaultdict

logfile = "log.txt"

pattern = re.compile(
    r"timestamp:\s*(\d+).*thread_id:\s*(\d+).*orderbook_id:\s*(\d+).*message_type:\s*(\w).*location:\s*(\d+)"
    # r"timestamp:\s*(\d+).*orderbook_id:\s*(\d+).*message_type:\s*(\w).*location:\s*(\d+)"
)

entries = []

# Parse log
with open(logfile) as f:
    for line in f:
        m = pattern.search(line)
        if not m:
            continue

        timestamp = int(m.group(1))
        thread_id = int(m.group(2))
        orderbook_id = int(m.group(3))
        message_type = m.group(4)
        location = int(m.group(5))

        entries.append((thread_id, orderbook_id, message_type, timestamp, location))


# Sort by thread_id then timestamp
entries.sort(key=lambda x: (x[0], x[3]))

latencies = defaultdict(list)

prev = None

for thread_id, orderbook_id, message_type, timestamp, location in entries:

    if prev:
        p_thread, p_ob, p_type, p_ts, p_loc = prev

        if (
            p_thread == thread_id
            and p_ob == orderbook_id
            and p_type == message_type
        ):
            key = (orderbook_id, message_type, f"{p_loc}-{location}")
            if key[2] != "5-1" and (key[2] == "3-4" or key[2] == "4-5" or key[2] == "1-5"):
                latencies[key].append(timestamp - p_ts)

    prev = (thread_id, orderbook_id, message_type, timestamp, location)


# Print averages
for (orderbook_id, message_type, pair), vals in sorted(latencies.items()):
    avg = sum(vals) / len(vals)
    print(
        f"orderbook={orderbook_id} "
        f"type={message_type} "
        f"loc={pair} "
        f"avg={avg:.1f}ns "
        f"count={len(vals)}"
    )
