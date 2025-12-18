#!/bin/bash
# Usage: bash run_nebula.sh <project.zip> <make-target>

ZIP_FILE=$1
TARGET=$2
LOG_FILE=nebula.log
DATA_DIR=data

# Function: Copy ZIP file to Nebula
function copy_to_nebula {
    echo "Copying project zip to Nebula..."
    scp $ZIP_FILE nebula:~/test/$ZIP_FILE || {
        echo "Error: Failed to copy project zip to Nebula"
        exit 1
    }
}

# Function: Clean the test directory on Nebula
function clean_test_dir {
    echo "Cleaning test directory on Nebula..."
    ssh nebula "mkdir -p ~/test && rm -rf ~/test/*" || {
        echo "Error: Failed to clean test directory on Nebula"
        exit 1
    }
}

# Function: Compile on Nebula and check compatibility
function compile_on_nebula {
    echo "Testing compilation on Nebula..."
    ssh nebula "\
        cd test && \
        unzip -u $ZIP_FILE -d uut && \
        cd uut/final && \
        mkdir -p data && \
        make clean && make all" || {
        echo "Error: Compilation on Nebula failed"
        exit 1
    }
    echo "Compilation on Nebula completed successfully!"
}

# Function: Run benchmarks on Nebula
function run_benchmarks_on_nebula {
    echo "Running benchmarks on Nebula..."
    ssh nebula "\
        cd test/uut/final && \
        /usr/local/slurm/bin/srun -t 2 -p q_student make $TARGET" | tee $LOG_FILE || { 
            echo "Error during benchmark execution on Nebula"; 
            exit 1; 
        }
    echo "Benchmarks completed successfully on Nebula!"
}

# Function: Copy results from Nebula
function copy_from_nebula {
    echo "Fetching benchmark results from Nebula..."
    mkdir -p $DATA_DIR
    scp -r 'nebula:~/test/uut/final/data/*' $DATA_DIR || {
        echo "Error: No benchmark results found in ~/test/uut/final/data/"
        exit 1
    }
    echo "Benchmark results copied successfully!"
}

# Function: Generate plots locally
function generate_plots {
    echo "Generating plots from benchmark results locally..."
    python3 benchmark.py || {
        echo "Error: Failed to generate plots"
        exit 1
    }
    echo "Plots generated successfully! Check the 'plots' directory."
}


# Check for valid arguments
if [ $# -ne 2 ]; then
    echo "Error: Provide a .zip archive as the first argument and a valid make target as the second (e.g., small-bench)."
    exit 1
fi

# Main script execution
clean_test_dir &&
copy_to_nebula &&
compile_on_nebula &&
run_benchmarks_on_nebula &&
copy_from_nebula &&
generate_plots

echo "All steps completed successfully! Results and plots are available in the $DATA_DIR directory."
