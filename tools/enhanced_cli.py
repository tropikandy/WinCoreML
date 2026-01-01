#!/usr/bin/env python3
"""
CoreMLWin Enhanced CLI - Beautiful command-line interface with UI polish

Features:
- Colored output
- Progress bars
- Spinners for long operations  
- ASCII art branding
- Beautiful tables
- Rich formatting
"""

import sys
import time
from pathlib import Path

# Add SDK to path
sdk_path = Path(__file__).parent.parent / "sdk" / "python"
sys.path.insert(0, str(sdk_path))

try:
    from coremlwin_client import CoreMLWinClient, CoreMLWinError
    import numpy as np
except ImportError as e:
    print(f"❌ Error: Failed to import SDK: {e}")
    sys.exit(1)


# ============================================================================
# Colors and Formatting
# ============================================================================

class Colors:
    """ANSI color codes"""
    RESET = '\033[0m'
    BOLD = '\033[1m'
    DIM = '\033[2m'
    
    # Foreground colors
    BLACK = '\033[30m'
    RED = '\033[31m'
    GREEN = '\033[32m'
    YELLOW = '\033[33m'
    BLUE = '\033[34m'
    MAGENTA = '\033[35m'
    CYAN = '\033[36m'
    WHITE = '\033[37m'
    
    # Bright foreground
    BRIGHT_BLACK = '\033[90m'
    BRIGHT_RED = '\033[91m'
    BRIGHT_GREEN = '\033[92m'
    BRIGHT_YELLOW = '\033[93m'
    BRIGHT_BLUE = '\033[94m'
    BRIGHT_MAGENTA = '\033[95m'
    BRIGHT_CYAN = '\033[96m'
    BRIGHT_WHITE = '\033[97m'
    
    # Background colors
    BG_BLACK = '\033[40m'
    BG_RED = '\033[41m'
    BG_GREEN = '\033[42m'
    BG_YELLOW = '\033[43m'
    BG_BLUE = '\033[44m'
    BG_MAGENTA = '\033[45m'
    BG_CYAN = '\033[46m'
    BG_WHITE = '\033[47m'


def colored(text, color):
    """Return colored text"""
    return f"{color}{text}{Colors.RESET}"


def success(text):
    """Green success message"""
    return colored(f"✓ {text}", Colors.BRIGHT_GREEN)


def error(text):
    """Red error message"""
    return colored(f"✗ {text}", Colors.BRIGHT_RED)


def warning(text):
    """Yellow warning message"""
    return colored(f"⚠ {text}", Colors.BRIGHT_YELLOW)


def info(text):
    """Cyan info message"""
    return colored(f"ℹ {text}", Colors.BRIGHT_CYAN)


def header(text):
    """Bold header"""
    return colored(text, Colors.BOLD + Colors.BRIGHT_WHITE)


# ============================================================================
# Spinners and Progress
# ============================================================================

class Spinner:
    """Animated spinner for long operations"""
    
    frames = ['⠋', '⠙', '⠹', '⠸', '⠼', '⠴', '⠦', '⠧', '⠇', '⠏']
    
    def __init__(self, message="Loading"):
        self.message = message
        self.running = False
        self.frame_idx = 0
    
    def __enter__(self):
        self.running = True
        return self
    
    def __exit__(self, *args):
        self.running = False
        print('\r' + ' ' * (len(self.message) + 10) + '\r', end='', flush=True)
    
    def update(self, message=None):
        if message:
            self.message = message
        if self.running:
            frame = self.frames[self.frame_idx % len(self.frames)]
            print(f'\r{colored(frame, Colors.BRIGHT_CYAN)} {self.message}', end='', flush=True)
            self.frame_idx += 1


def progress_bar(current, total, width=50):
    """Display a progress bar"""
    percent = current / total
    filled = int(width * percent)
    bar = '█' * filled + '░' * (width - filled)
    percentage = f"{percent*100:.1f}%"
    return f"{colored(bar, Colors.BRIGHT_GREEN)} {percentage}"


# ============================================================================
# ASCII Art Branding
# ============================================================================

LOGO = f"""{Colors.BRIGHT_CYAN}
 ██████╗ ██████╗ ██████╗ ███████╗███╗   ███╗██╗     ██╗    ██╗██╗███╗   ██╗
██╔════╝██╔═══██╗██╔══██╗██╔════╝████╗ ████║██║     ██║    ██║██║████╗  ██║
██║     ██║   ██║██████╔╝█████╗  ██╔████╔██║██║     ██║ █╗ ██║██║██╔██╗ ██║
██║     ██║   ██║██╔══██╗██╔══╝  ██║╚██╔╝██║██║     ██║███╗██║██║██║╚██╗██║
╚██████╗╚██████╔╝██║  ██║███████╗██║ ╚═╝ ██║███████╗╚███╔███╔╝██║██║ ╚████║
 ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝╚══════╝ ╚══╝╚══╝ ╚═╝╚═╝  ╚═══╝
{Colors.RESET}
{colored('Universal ML Runtime for Windows with DirectML GPU Acceleration', Colors.BRIGHT_WHITE)}
{colored('━' * 80, Colors.BRIGHT_BLACK)}
"""


# ============================================================================
# Beautiful Tables
# ============================================================================

def print_table(headers, rows, col_widths=None):
    """Print a beautiful formatted table"""
    if not col_widths:
        col_widths = [max(len(str(row[i])) for row in [headers] + rows) + 2 for i in range(len(headers))]
    
    # Top border
    print(colored('┌' + '┬'.join('─' * w for w in col_widths) + '┐', Colors.BRIGHT_BLACK))
    
    # Header
    header_row = '│'.join(colored(str(headers[i]).center(col_widths[i]), Colors.BOLD + Colors.BRIGHT_WHITE) for i in range(len(headers)))
    print(colored('│', Colors.BRIGHT_BLACK) + header_row + colored('│', Colors.BRIGHT_BLACK))
    
    # Header separator
    print(colored('├' + '┼'.join('─' * w for w in col_widths) + '┤', Colors.BRIGHT_BLACK))
    
    # Rows
    for row in rows:
        row_str = '│'.join(str(row[i]).ljust(col_widths[i]) for i in range(len(row)))
        print(colored('│', Colors.BRIGHT_BLACK) + row_str + colored('│', Colors.BRIGHT_BLACK))
    
    # Bottom border
    print(colored('└' + '┴'.join('─' * w for w in col_widths) + '┘', Colors.BRIGHT_BLACK))


# ============================================================================
# Enhanced Commands
# ============================================================================

def cmd_health():
    """Check service health with beautiful output"""
    print(LOGO)
    
    client = CoreMLWinClient()
    
    with Spinner("Connecting to CoreMLWin service") as spinner:
        for i in range(10):
            spinner.update()
            time.sleep(0.1)
        
        try:
            health = client.health()
        except Exception as e:
            print(error(f"Failed to connect: {e}"))
            return 1
    
    # Display health status
    print()
    print(header("Service Health"))
    print(colored('━' * 80, Colors.BRIGHT_BLACK))
    
    status = success("RUNNING") if health.get('ready') else error("DOWN")
    version = health.get('version', 'unknown')
    
    print(f"Status:  {status}")
    print(f"Version: {colored(version, Colors.BRIGHT_CYAN)}")
    print()
    
    return 0


def cmd_monitor():
    """Monitor service in real-time with live updates"""
    print(LOGO)
    print(info("Monitoring CoreMLWin service (Ctrl+C to stop)"))
    print()
    
    client = CoreMLWinClient()
    
    try:
        while True:
            timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
            
            try:
                health = client.health()
                status = success("✓ RUNNING") if health.get('ready') else error("✗ DOWN")
                version = health.get('version', 'unknown')
                
                # Clear line and print status
                print(f'\r{colored(timestamp, Colors.DIM)} {status} (v{version})', end='', flush=True)
                
            except Exception as e:
                print(f'\r{colored(timestamp, Colors.DIM)} {error("CONNECTION FAILED")}', end='', flush=True)
            
            time.sleep(1)
    
    except KeyboardInterrupt:
        print('\n\n' + info("Monitoring stopped"))
        return 0


def cmd_list():
    """List models with beautiful table"""
    print(LOGO)
    
    client = CoreMLWinClient()
    
    with Spinner("Fetching models") as spinner:
        for i in range(5):
            spinner.update()
            time.sleep(0.1)
        
        try:
            models = client.list_models()
        except Exception as e:
            print(error(f"Failed to fetch models: {e}"))
            return 1
    
    print()
    
    if not models:
        print(info("No models registered"))
        return 0
    
    print(header(f"Registered Models ({len(models)})"))
    print(colored('━' * 80, Colors.BRIGHT_BLACK))
    print()
    
    headers = ['Model ID', 'Provider', 'Inputs', 'Outputs']
    rows = [
        [
            m.get('model_id', 'unknown')[:20] + '...' if len(m.get('model_id', '')) > 20 else m.get('model_id', ''),
            colored(m.get('provider', 'CPU'), Colors.BRIGHT_YELLOW),
            str(len(m.get('input_shapes', {}))),
            str(len(m.get('output_shapes', {})))
        ]
        for m in models
    ]
    
    print_table(headers, rows)
    print()
    
    return 0


def cmd_benchmark():
    """Run performance benchmark with progress bar"""
    print(LOGO)
    print(header("Performance Benchmark"))
    print(colored('━' * 80, Colors.BRIGHT_BLACK))
    print()
    
    client = CoreMLWinClient()
    
    # Warmup
    print(info("Warmup phase..."))
    warmup_iters = 5
    for i in range(warmup_iters):
        print(f'\r{progress_bar(i+1, warmup_iters)}', end='', flush=True)
        client.health()
        time.sleep(0.1)
    print()
    
    # Benchmark
    print(info("Running benchmark..."))
    bench_iters = 50
    latencies = []
    
    for i in range(bench_iters):
        print(f'\r{progress_bar(i+1, bench_iters)}', end='', flush=True)
        
        start = time.time()
        client.health()
        latency = (time.time() - start) * 1000
        latencies.append(latency)
    
    print('\n')
    
    # Results
    avg = sum(latencies) / len(latencies)
    min_lat = min(latencies)
    max_lat = max(latencies)
    p95 = sorted(latencies)[int(len(latencies) * 0.95)]
    
    print(header("Results"))
    print(colored('━' * 80, Colors.BRIGHT_BLACK))
    print(f"Mean latency:   {colored(f'{avg:.2f} ms', Colors.BRIGHT_GREEN)}")
    print(f"Min latency:    {colored(f'{min_lat:.2f} ms', Colors.BRIGHT_CYAN)}")
    print(f"Max latency:    {colored(f'{max_lat:.2f} ms', Colors.BRIGHT_YELLOW)}")
    print(f"P95 latency:    {colored(f'{p95:.2f} ms', Colors.BRIGHT_MAGENTA)}")
    print(f"Throughput:     {colored(f'{1000/avg:.1f} req/s', Colors.BRIGHT_GREEN)}")
    print()
    
    return 0


# ============================================================================
# Main
# ============================================================================

def main():
    import argparse
    
    parser = argparse.ArgumentParser(
        description='CoreMLWin Enhanced CLI',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='For more information, visit: https://github.com/user/WinCoreML'
    )
    
    subparsers = parser.add_subparsers(dest='command', help='Commands')
    
    # Commands
    subparsers.add_parser('health', help='Check service health')
    subparsers.add_parser('monitor', help='Monitor service in real-time')
    subparsers.add_parser('list', help='List all registered models')
    subparsers.add_parser('benchmark', help='Run performance benchmark')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    # Dispatch command
    commands = {
        'health': cmd_health,
        'monitor': cmd_monitor,
        'list': cmd_list,
        'benchmark': cmd_benchmark,
    }
    
    return commands[args.command]()


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print('\n' + info("Interrupted by user"))
        sys.exit(130)
    except Exception as e:
        print(error(f"Unexpected error: {e}"))
        sys.exit(1)
