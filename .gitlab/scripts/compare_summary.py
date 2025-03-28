import pandas as pd
import sys
import argparse
import logging
from colorama import Fore, Style

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

def compare_trace_files(file1, file2):
    logging.info("Starting comparison of trace files.")
    
    # Load the CSV files into DataFrames
    logging.info(f"Loading file: {file1}")
    df1 = pd.read_csv(file1)
    logging.info(f"Loading file: {file2}")
    df2 = pd.read_csv(file2)

    # Merge the two DataFrames on workload_name and num_nodes
    logging.info("Merging the two DataFrames on 'workload_name' and 'num_nodes'.")
    merged_df = pd.merge(df1, df2, on=['workload_name', 'num_nodes'], how='inner', suffixes=('_file1', '_file2'))

    # Calculate percentage difference for num_events and trace_size_bytes
    logging.info("Calculating percentage differences for 'num_events' and 'trace_size_bytes'.")
    merged_df['num_events_diff_%'] = ((merged_df['num_events_file2'] - merged_df['num_events_file1']) / 
                                      merged_df['num_events_file1']) * 100
    merged_df['trace_size_bytes_diff_%'] = ((merged_df['trace_size_bytes_file2'] - merged_df['trace_size_bytes_file1']) / 
                                            merged_df['trace_size_bytes_file1']) * 100

    # Select relevant columns to display
    logging.info("Selecting relevant columns for the result.")
    result = merged_df[['workload_name', 'num_nodes', 'num_events_diff_%', 'trace_size_bytes_diff_%']]

    logging.info("Comparison completed successfully.")
    return result

def color_code_output(row):
    def colorize(value):
        if abs(value) < 5:
            return f"{Fore.GREEN}{value:.2f}%{Style.RESET_ALL}"
        elif 5 <= abs(value) < 10:
            return f"{Fore.YELLOW}{value:.2f}%{Style.RESET_ALL}"
        else:
            return f"{Fore.RED}{value:.2f}%{Style.RESET_ALL}"

    return [
        row['workload_name'],
        row['num_nodes'],
        colorize(row['num_events_diff_%']),
        colorize(row['trace_size_bytes_diff_%'])
    ]

if __name__ == "__main__":
    # File paths for the two trace_paths.csv files
    parser = argparse.ArgumentParser(description="Compare two trace files and calculate percentage differences.")
    parser.add_argument("file1", type=str, help="Path to the first trace file (CSV format).")
    parser.add_argument("file2", type=str, help="Path to the second trace file (CSV format).")
    parser.add_argument("--output_file", type=str, default="output.csv", help="Path to the output file (default: output.csv in the current directory).")
    args = parser.parse_args()

    file1 = args.file1
    file2 = args.file2

    logging.info("Script started.")
    logging.info(f"File 1: {file1}")
    logging.info(f"File 2: {file2}")

    # Compare the files and display the result
    try:
        # Compare the files
        comparison_result = compare_trace_files(file1, file2)
        
        # Display the result in the console
        print(f"{'Workload Name':<20} {'Num Nodes':<10} {'Num Events Diff %':<20} {'Trace Size Diff %':<20}")
        print("-" * 70)
        for _, row in comparison_result.iterrows():
            colored_row = color_code_output(row)
            print(f"{colored_row[0]:<20} {colored_row[1]:<10} {colored_row[2]:<20} {colored_row[3]:<20}")
        
        # Save the result to the specified file
        comparison_result.to_csv(args.output_file, index=False)
        logging.info(f"Comparison result saved to {args.output_file}")
    except Exception as e:
        logging.error(f"An error occurred: {e}")
    
    logging.info("Script finished.")