#!/usr/bin/env python
"""
Universal ML Runtime CLI

Command-line interface for managing models and monitoring the runtime service.

Usage:
    coremlwin_cli.py list                          - List all registered models
    coremlwin_cli.py register <model_path>         - Register a new model
    coremlwin_cli.py unregister <model_id>         - Unregister a model
    coremlwin_cli.py info <model_id>               - Get model information
    coremlwin_cli.py predict <model_id> <input>    - Run prediction
    coremlwin_cli.py benchmark <model_id>          - Show benchmark results
    coremlwin_cli.py health                        - Check service health
    coremlwin_cli.py monitor                       - Monitor service in real-time
"""

import sys
import argparse
import json
from pathlib import Path
import time

# Add SDK to path
sdk_path = Path(__file__).parent.parent / "sdk" / "python"
sys.path.insert(0, str(sdk_path))

try:
    from coreml_win import RuntimeClient
    from coreml_win.errors import CoreMLWinError
    import numpy as np
except ImportError as e:
    print(f"Error: Failed to import SDK: {e}")
    print("Make sure the Python SDK is installed:")
    print("  cd sdk/python && pip install -e .")
    sys.exit(1)


def cmd_list(args, client):
    """List all registered models."""
    try:
        models = client.list_models()

        if not models:
            print("No models registered.")
            return 0

        print(f"\n{'Model ID':<40} {'Format':<12} {'Inputs':<20} {'Outputs'}")
        print("-" * 100)

        for model in models:
            model_id = model.get('model_id', 'unknown')
            model_format = model.get('model_format', 'unknown')
            inputs = ', '.join(model.get('input_names', []))
            outputs = ', '.join(model.get('output_names', []))

            print(f"{model_id:<40} {model_format:<12} {inputs:<20} {outputs}")

        print(f"\nTotal: {len(models)} model(s)")
        return 0

    except CoreMLWinError as e:
        print(f"Error: {e}")
        return 1


def cmd_register(args, client):
    """Register a new model."""
    model_path = Path(args.model_path)

    if not model_path.exists():
        print(f"Error: Model file not found: {model_path}")
        return 1

    print(f"Registering model: {model_path}")
    print("This may take a while (conversion + benchmarking)...")

    try:
        start = time.time()
        model_id = client.register_model(str(model_path))
        duration = time.time() - start

        print(f"\n✓ Model registered successfully!")
        print(f"  Model ID: {model_id}")
        print(f"  Duration: {duration:.2f}s")

        return 0

    except CoreMLWinError as e:
        print(f"Error: {e}")
        return 1


def cmd_unregister(args, client):
    """Unregister a model."""
    try:
        success = client.unregister_model(args.model_id)

        if success:
            print(f"✓ Model unregistered: {args.model_id}")
            return 0
        else:
            print(f"✗ Failed to unregister model")
            return 1

    except CoreMLWinError as e:
        print(f"Error: {e}")
        return 1


def cmd_info(args, client):
    """Get detailed model information."""
    try:
        info = client.get_model_info(args.model_id)

        print(f"\nModel Information:")
        print(f"  Model ID:     {info.get('model_id', 'unknown')}")
        print(f"  Format:       {info.get('format', 'unknown')}")
        print(f"  Input names:  {', '.join(info.get('input_names', []))}")
        print(f"  Output names: {', '.join(info.get('output_names', []))}")

        if 'input_shapes' in info:
            print(f"\n  Input shapes:")
            for name, shape in info['input_shapes'].items():
                print(f"    {name}: {shape}")

        if 'output_shapes' in info:
            print(f"\n  Output shapes:")
            for name, shape in info['output_shapes'].items():
                print(f"    {name}: {shape}")

        if 'benchmark' in info:
            cmd_benchmark_display(info['benchmark'])

        return 0

    except CoreMLWinError as e:
        print(f"Error: {e}")
        return 1


def cmd_benchmark_display(benchmark):
    """Display benchmark results."""
    print(f"\n  Benchmark Results:")
    print(f"    Fastest provider: {benchmark.get('fastest_provider', 'unknown')}")
    print(f"    Best latency:     {benchmark.get('best_latency_ms', 0):.2f} ms")
    print(f"    CPU baseline:     {benchmark.get('baseline_latency_ms', 0):.2f} ms")
    print(f"    Speedup:          {benchmark.get('speedup_vs_cpu', 1):.2f}x")

    if 'results' in benchmark:
        print(f"\n    Per-Provider Results:")
        print(f"    {'Provider':<30} {'Latency (ms)':<15} {'Status'}")
        print(f"    {'-' * 60}")

        for result in benchmark['results']:
            provider = result.get('provider', 'unknown')
            latency = result.get('mean_latency_ms', 0)
            success = "✓" if result.get('success', False) else "✗"

            print(f"    {provider:<30} {latency:<15.2f} {success}")


def cmd_health(args, client):
    """Check service health."""
    try:
        health = client.health()

        print(f"\nService Health:")
        print(f"  Version: {health.get('version', 'unknown')}")
        print(f"  Ready:   {'✓ Yes' if health.get('ready', False) else '✗ No'}")

        return 0 if health.get('ready', False) else 1

    except Exception as e:
        print(f"Error: Service not responding: {e}")
        return 1


def cmd_monitor(args, client):
    """Monitor service in real-time."""
    print("Monitoring Universal ML Runtime")
    print("Press Ctrl+C to stop\n")

    try:
        while True:
            try:
                health = client.health()
                timestamp = time.strftime("%H:%M:%S")

                status = "✓ RUNNING" if health.get('ready', False) else "✗ DOWN"
                version = health.get('version', 'unknown')

                print(f"[{timestamp}] {status} (v{version})", end='\r')

                time.sleep(args.interval)

            except KeyboardInterrupt:
                print("\n\nMonitoring stopped.")
                return 0
            except Exception as e:
                print(f"\n[{time.strftime('%H:%M:%S')}] Error: {e}", end='\r')
                time.sleep(args.interval)

    except KeyboardInterrupt:
        print("\n\nMonitoring stopped.")
        return 0


def main():
    parser = argparse.ArgumentParser(
        description="Universal ML Runtime CLI",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # List all models
  coremlwin_cli.py list

  # Register a model
  coremlwin_cli.py register models/resnet50.pt

  # Get model info
  coremlwin_cli.py info abc123def456

  # Monitor service
  coremlwin_cli.py monitor --interval 5
        """
    )

    parser.add_argument('--pipe', default=r"\\.\pipe\coremlwin_runtime",
                       help='Named pipe path')

    subparsers = parser.add_subparsers(dest='command', help='Command to execute')

    # List command
    subparsers.add_parser('list', help='List all registered models')

    # Register command
    register_parser = subparsers.add_parser('register', help='Register a new model')
    register_parser.add_argument('model_path', help='Path to model file')

    # Unregister command
    unregister_parser = subparsers.add_parser('unregister', help='Unregister a model')
    unregister_parser.add_argument('model_id', help='Model ID to unregister')

    # Info command
    info_parser = subparsers.add_parser('info', help='Get model information')
    info_parser.add_argument('model_id', help='Model ID')

    # Health command
    subparsers.add_parser('health', help='Check service health')

    # Monitor command
    monitor_parser = subparsers.add_parser('monitor', help='Monitor service')
    monitor_parser.add_argument('--interval', type=int, default=5,
                                help='Refresh interval in seconds (default: 5)')

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return 1

    # Create client
    try:
        client = RuntimeClient(pipe_name=args.pipe)
    except Exception as e:
        print(f"Error: Failed to create client: {e}")
        return 1

    # Dispatch command
    commands = {
        'list': cmd_list,
        'register': cmd_register,
        'unregister': cmd_unregister,
        'info': cmd_info,
        'health': cmd_health,
        'monitor': cmd_monitor,
    }

    if args.command in commands:
        return commands[args.command](args, client)
    else:
        print(f"Error: Unknown command: {args.command}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
