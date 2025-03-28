#!/bin/bash

echo "Starting generate_summary.sh on $(hostname)" | tee -a "$LOG_FILE"

echo "Exporting DFTRACER_VERSION..."
export DFTRACER_VERSION=$(python -c "import dftracer; print(dftracer.__version__)") || { echo "Failed to get DFTRACER_VERSION"; exit 1; }
echo "DFTRACER_VERSION: $DFTRACER_VERSION"

LFS_DIR=v$DFTRACER_VERSION/$SYSTEM_NAME
ROOT_PATH=$LOG_STORE_DIR/$LFS_DIR
CSV_FILE=$ROOT_PATH/trace_paths.csv
COMPARE_CSV_FILE=$ROOT_PATH/compare.csv

echo "Setting up CSV file at $CSV_FILE..."
# Create/overwrite CSV file with header
echo "workload_name,num_nodes,ci_date,trace_path,trace_size_bytes,trace_size_fmt,num_events" > "$CSV_FILE"

echo "Starting traversal of workload directories in $ROOT_PATH..."
# Traverse through workload directories
for workload_path in "$ROOT_PATH"/*; do
    if [ ! -d "$workload_path" ]; then
        echo "Skipping non-directory: $workload_path"
        continue
    fi
    workload_name=$(basename "$workload_path")
    echo "Processing workload: $workload_name"
    
    # For each node configuration
    for node_path in "$workload_path"/nodes-*; do
        if [ -d "$node_path" ]; then
            node_config=$(basename "$node_path")
            node_num=${node_config#nodes-}  # Remove 'nodes-' prefix
            echo "Processing node configuration: $node_config (Nodes: $node_num)"
            
            # Find the latest timestamp directory
            latest_timestamp=$(find "$node_path" -mindepth 1 -maxdepth 1 -type d -exec basename {} \; | sort -r | head -n 1)
            
            if [ -n "$latest_timestamp" ]; then
                echo "Latest timestamp found: $latest_timestamp"
                full_path="$node_path/$latest_timestamp/RAW"
                if [ -d "$full_path" ]; then
                    echo "Found trace path: $full_path"
                    shopt -s nullglob
                    zindex_files=("$full_path"/*.zindex)
                    if [ ${#zindex_files[@]} -eq 0 ]; then
                        echo "No index found in: $full_path"
                        dftracer_create_index -f -d "$full_path/"
                    else 
                        echo "Index found in: $full_path"
                    fi
                    # Get size information
                    size_bytes=$(du -b "$full_path" | cut -f1)
                    size_formatted=$(du -sh "$full_path" | cut -f1)
                    event_counts=$(dftracer_event_count -d "$full_path")
                    
                    echo "Size: $size_formatted"
                    echo "----------------------------------------"
                    # Calculate the relative path of full_path based on ROOT_PATH
                    relative_path=$(realpath --relative-to="$ROOT_PATH" "$full_path")
                    echo "Relative path: $relative_path"
                    # Write to CSV file with size information
                    echo "$workload_name,$node_num,$latest_timestamp,$relative_path,$size_bytes,$size_formatted,$event_counts" >> "$CSV_FILE"
                else
                    echo "No COMPACT directory found at $full_path"
                fi
            else
                echo "No timestamp directories found in $node_path"
            fi
        else
            echo "Skipping non-directory: $node_path"
        fi
    done
done

num_lines=$(wc -l < "$CSV_FILE")
echo "CSV file created at: $CSV_FILE with $num_lines lines"

echo "Sorting CSV file by workload_name and num_nodes..."
# Sort the CSV file by workload_name (alphabetically) and num_nodes (numerically)
header=$(head -n 1 "$CSV_FILE")
tail -n +2 "$CSV_FILE" | sort -t, -k1,1 -k2,2n > "${CSV_FILE}.sorted"
echo "$header" > "$CSV_FILE"
cat "${CSV_FILE}.sorted" >> "$CSV_FILE"
rm "${CSV_FILE}.sorted"
echo "CSV file sorted successfully."

if [[ $num_lines -eq 1 ]]; then
    echo "No trace paths found. Cleaning up..."
    rm -rf $ROOT_PATH
    exit 1
else 
    echo "Preparing to commit and push files..."
    cd $ROOT_PATH
    git add .gitattributes .gitignore prepare_traces.sh README.md trace_paths.csv 
    git commit -m "added initial files" || { echo "Failed to commit files"; exit 1; }
    git push origin "$LFS_DIR" || { echo "Failed to push branch $LFS_DIR"; exit 1; }
    echo "Files committed and pushed successfully."
    cd - || { echo "Failed to return to previous directory"; exit 1; }
fi

echo "generate_summary.sh completed."

BASELINE=/p/lustre3/iopp/dftracer-traces-lfs/v1.0.10.dev6/corona/trace_paths.csv
echo "Starting comparison of summary files..."
python .gitlab/scripts/compare_summary.py ${BASELINE} "$CSV_FILE" --output_file "$COMPARE_CSV_FILE"
echo "Comparison completed. Output written to $COMPARE_CSV_FILE"
