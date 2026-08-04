#!/usr/bin/env python3
"""
Generate random malloc/free traces in the format expected by the trace runner.

Format:
  Line 1: Initial heap size
  Line 2: Total number of operations
  Line 3: Number of allocations
  Line 4: Number of frees
  Following lines: Operations
    a <id> <size>  - allocate
    f <id>         - free
"""

import random
import sys
import argparse


def generate_trace(num_ops, min_size, max_size, seed=None):
    """Generate a random trace with num_ops operations."""
    if seed is not None:
        random.seed(seed)

    operations = []
    allocated = {}
    next_id = 0

    for _ in range(num_ops):
        if allocated and random.random() < 0.4:  # 40% chance to free
            # Free a random allocated block
            alloc_id = random.choice(list(allocated.keys()))
            operations.append(("f", alloc_id))
            del allocated[alloc_id]
        else:
            # Allocate a new block
            size_float = random.random() ** 2
            size = (max_size - min_size) * size_float + min_size
            operations.append(("a", next_id, int(size)))
            allocated[next_id] = size
            next_id += 1

    # Free remaining allocated blocks at the end
    for alloc_id in sorted(allocated.keys()):
        operations.append(("f", alloc_id))

    num_allocs = sum(1 for op in operations if op[0] == "a")
    num_frees = sum(1 for op in operations if op[0] == "f")

    return operations, num_allocs, num_frees


def write_trace(output_file, operations, num_allocs, num_frees):
    """Write trace to file in the expected format."""
    with open(output_file, "w") as f:
        total_ops = len(operations)
        f.write(f"{total_ops}\n")
        f.write(f"{num_allocs}\n")
        f.write(f"{num_frees}\n")

        for op in operations:
            if op[0] == "a":
                f.write(f"a {op[1]} {op[2]}\n")
            else:
                f.write(f"f {op[1]}\n")


def main():
    parser = argparse.ArgumentParser(description="Generate random malloc/free traces")
    parser.add_argument(
        "-n",
        "--num-ops",
        type=int,
        default=100,
        help="Number of operations to generate (default: 100)",
    )
    parser.add_argument(
        "-s",
        "--min-size",
        type=int,
        default=16,
        help="Minimum size of a single allocation (default: 16)",
    )
    parser.add_argument(
        "-m",
        "--max-size",
        type=int,
        default=8192,
        help="Maximum size of a single allocation (default: 8192)",
    )
    parser.add_argument(
        "-o", "--output", type=str, required=True, help="Output file for the trace"
    )
    parser.add_argument(
        "--seed", type=int, default=None, help="Random seed for reproducibility"
    )

    args = parser.parse_args()

    operations, num_allocs, num_frees = generate_trace(
        args.num_ops, args.min_size, args.max_size, args.seed
    )

    write_trace(args.output, operations, num_allocs, num_frees)
    print(f"Generated trace with {len(operations)} operations to {args.output}")


if __name__ == "__main__":
    main()
