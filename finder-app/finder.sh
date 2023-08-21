#!/bin/sh

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
    file="$1"
    count=0

    while IFS= read -r line; do
        case $line in
            *"$searchstr"*) count=$((count + 1));;
        esac
    done < "$file"

    echo "$count"
}

# Recursive function to process directory
process_directory() {
    dir="$1"
    total_files=0
    total_matching_lines=0

    # Loop through files and subdirectories in the directory
    for item in "$dir"/*; do
        if [ -f "$item" ]; then
            total_files=$((total_files + 1))
            matching_lines=$(count_matching_lines "$item")
            total_matching_lines=$((total_matching_lines + matching_lines))
        elif [ -d "$item" ]; then
            sub_totals=$(process_directory "$item")
            total_files=$((total_files + $(echo "$sub_totals" | cut -d' ' -f1)))
            total_matching_lines=$((total_matching_lines + $(echo "$sub_totals" | cut -d' ' -f2)))
        fi
    done

    echo "$total_files $total_matching_lines"
}

# Main script execution starts here
result=$(process_directory "$filesdir")
total_files=$(echo "$result" | cut -d' ' -f1)
total_matching_lines=$(echo "$result" | cut -d' ' -f2)

echo "The number of files are $total_files and the number of matching lines are $total_matching_lines"
