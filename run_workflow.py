import argparse
import os
import subprocess
import sys

def run_command(command):
    """Run a command and return its exit code"""
    print(f"\nExecuting: {command}")
    result = subprocess.run(command, shell=True)
    return result.returncode

def main():
    """Run the tomato disease detection workflow with configurable options"""
    parser = argparse.ArgumentParser(description="Tomato Disease Detection Workflow")
    
    # Add workflow steps as options
    parser.add_argument("--preprocess", action="store_true", help="Run the preprocessing step")
    parser.add_argument("--augment", action="store_true", help="Run the augmentation step")
    parser.add_argument("--prepare", action="store_true", help="Run the dataset preparation step")
    parser.add_argument("--train", action="store_true", help="Run the model training step")
    parser.add_argument("--all", action="store_true", help="Run all steps in sequence")
    
    # Add augmentation options
    parser.add_argument("--samples", type=int, default=3, 
                        help="Number of augmented samples per image (default: 3)")
    
    # Add custom directory options
    parser.add_argument("--raw_dir", type=str, 
                        help="Custom raw dataset directory (default: raw_dataset)")
    parser.add_argument("--processed_dir", type=str, 
                        help="Custom processed dataset directory (default: processed_dataset)")
    parser.add_argument("--augmented_dir", type=str, 
                        help="Custom augmented dataset directory (default: augmented_dataset)")
    parser.add_argument("--data_dir", type=str, 
                        help="Custom data directory for training (default: data)")
    
    # Parse arguments
    args = parser.parse_args()
    
    # Check if at least one action is specified
    if not (args.preprocess or args.augment or args.prepare or args.train or args.all):
        parser.print_help()
        return 1
    
    # Prepare commands
    commands = []
    
    # Python executable - use the one that's running this script
    python = sys.executable
    
    # Add steps based on options
    if args.preprocess or args.all:
        cmd = f"{python} preprocess.py"
        if args.raw_dir:
            # Note: preprocess.py doesn't currently support custom dirs via args,
            # so we're noting this limitation
            print("Note: Custom raw_dir not supported in preprocess.py")
        commands.append(cmd)
    
    if args.augment or args.all:
        cmd = f"{python} augment_dataset.py --augment --samples {args.samples}"
        if args.raw_dir:
            cmd += f" --input_dir {args.raw_dir}"
        if args.augmented_dir:
            cmd += f" --output_dir {args.augmented_dir}"
        commands.append(cmd)
    
    if args.prepare or args.all:
        cmd = f"{python} prepare_dataset.py"
        # Note: prepare_dataset.py doesn't currently support custom dirs via args
        if args.raw_dir or args.data_dir:
            print("Note: Custom directories not supported in prepare_dataset.py")
        commands.append(cmd)
    
    if args.train or args.all:
        cmd = f"{python} train_model.py"
        # Note: train_model.py doesn't currently support custom dirs via args
        if args.processed_dir or args.data_dir:
            print("Note: Custom directories not supported in train_model.py")
        commands.append(cmd)
    
    # Execute commands
    for cmd in commands:
        exit_code = run_command(cmd)
        if exit_code != 0:
            print(f"\nError: Command failed with exit code {exit_code}")
            print(f"Failed command: {cmd}")
            return exit_code
    
    print("\nWorkflow completed successfully!")
    return 0

if __name__ == "__main__":
    sys.exit(main())
