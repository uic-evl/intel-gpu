#!/usr/bin/env python3
import multiprocessing
import numpy as np
import time
import argparse
from itertools import count
import os

def memory_stress(size_mb=4000):
    """Aggressively stress memory with large arrays and operations."""
    # Convert MB to bytes
    size = size_mb * 1024 * 1024
    
    try:
        while True:
            # Create multiple large arrays
            arrays = []
            for _ in range(4):  # Create 4 large arrays
                arrays.append(np.random.randn(size // 32))  # 8 bytes per float64
            
            # Continuous intensive operations
            while True:
                # Matrix operations
                for i in range(len(arrays)-1):
                    arrays[i] = np.dot(arrays[i], arrays[i+1])
                    arrays[i+1] = np.fft.fft2(arrays[i])
                    arrays[i] = np.sort(arrays[i])  # Memory intensive operation
                
                # Force cache misses with large strided access
                for arr in arrays:
                    arr[::64] = arr[::-64]
                
                # Random memory access patterns
                for arr in arrays:
                    idx = np.random.randint(0, len(arr), size=len(arr)//100)
                    arr[idx] = np.random.randn(len(idx))
    except:
        # Catch any memory errors and retry with smaller arrays
        time.sleep(1)
        memory_stress(size_mb // 2)

def cpu_stress():
    """Maximum CPU stress with AVX/SSE instructions."""
    x = np.random.randn(2**20)
    y = np.random.randn(2**20)
    
    while True:
        # Mix of floating-point and integer operations
        for _ in range(1000):
            # Vector operations to stress AVX units
            np.dot(x, y)
            np.exp(x)
            np.log(np.abs(y) + 1)
            
            # Integer operations
            for i in range(1000):
                pow(i, i % 100, 2**32-1)
            
            # Bitwise operations
            x = np.frombuffer(os.urandom(x.nbytes), dtype=x.dtype)
            y = np.frombuffer(os.urandom(y.nbytes), dtype=y.dtype)

def main():
    parser = argparse.ArgumentParser(description='Maximum System Stress Test')
    parser.add_argument('--cpus', type=int, default=multiprocessing.cpu_count(),
                       help='Number of CPU cores to stress (default: all)')
    parser.add_argument('--memory', type=int, default=4000,
                       help='Memory to use per process in MB (default: 4000)')
    parser.add_argument('--memory-processes', type=int, default=32,
                       help='Number of memory stress processes (default: 32)')
    args = parser.parse_args()

    total_processes = args.cpus
    memory_processes = min(args.memory_processes, total_processes - 1)
    cpu_processes = total_processes - memory_processes

    processes = []
    
    # Start memory stress processes
    print(f"Starting {memory_processes} memory stress processes...")
    print(f"Each memory process will attempt to use {args.memory}MB")
    print(f"Total memory target: {memory_processes * args.memory / 1024:.1f}GB")
    
    for _ in range(memory_processes):
        p = multiprocessing.Process(target=memory_stress, args=(args.memory,))
        p.start()
        processes.append(p)

    # Start CPU stress processes
    print(f"Starting {cpu_processes} CPU stress processes...")
    for _ in range(cpu_processes):
        p = multiprocessing.Process(target=cpu_stress)
        p.start()
        processes.append(p)

    print("\nMaximum stress test running!")
    print("Monitoring process health...")
    
    try:
        while True:
            alive_processes = sum(1 for p in processes if p.is_alive())
            if alive_processes < len(processes):
                print(f"\nWarning: {len(processes) - alive_processes} processes have died")
                print("System might be running out of memory or hitting thermal limits")
            time.sleep(5)
    except KeyboardInterrupt:
        print("\nStopping stress test...")
        for p in processes:
            p.terminate()
        for p in processes:
            p.join()
        print("Stress test stopped")

if __name__ == '__main__':
    main()