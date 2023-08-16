#!/bin/bash

# Check if the required arguments are provided
if [ $# -ne 2 ]; then
    echo "Usage: $0 <filesdir> <searchstr>"
    exit 1
fi

filesdir="$1"
searchstr="$2"

# Check if filesdir exists and is a directory
if [ ! -d "$filesdir" ]; then
    echo "Error: $filesdir is not a directory."
    exit 1
fi

# Function to count matching lines in a file
count_matching_lines() {
    local file="$1"
    local count=0

    while IFS= read -r line; do
        if [[ $line == *"$searchstr"* ]]; then
            ((count++))
        fi
    done < "$file"

    echo "$count"
}

# Recursive function to process directory
process_directory() {
    local dir="$1"
    local total_files=0
    local total_matching_lines=0

    # Loop through files and subdirectories in the directory
    for item in "$dir"/*; do
        if [ -f "$item" ]; then
            ((total_files++))
            matching_lines=$(count_matching_lines "$item")
            ((total_matching_lines += matching_lines))
        elif [ -d "$item" ]; then
            sub_totals=$(process_directory "$item")
            total_files=$((total_files + sub_totals[0]))
            total_matching_lines=$((total_matching_lines + sub_totals[1]))
        fi
    done

    echo "$total_files $total_matching_lines"
}

# Main script execution starts here
result=($(process_directory "$filesdir"))
total_files="${result[0]}"
total_matching_lines="${result[1]}"

echo "The number of files are $total_files and the number of matching lines are $total_matching_lines"
