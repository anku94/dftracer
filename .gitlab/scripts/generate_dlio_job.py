import os
import yaml
from pathlib import Path
import dlio_benchmark
import argparse
import uuid
import dftracer
from datetime import datetime
from tqdm import tqdm  # Import tqdm for progress tracking
import logging  # Import logging for detailed logs

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s",
)


# def str_presenter(dumper, data):
#     """configures yaml for dumping multiline strings
#     Ref: https://stackoverflow.com/questions/8640959/how-can-i-control-what-scalar-form-pyyaml-uses-for-my-data
#     """
#     if len(data.splitlines()) > 1:  # check for multiline string
#         return dumper.represent_scalar("tag:yaml.org,2002:str", data, style="|")
#     return dumper.represent_scalar("tag:yaml.org,2002:str", data)


# yaml.add_representer(str, str_presenter)
# yaml.representer.SafeRepresenter.add_representer(
#     str, str_presenter
# )  # to use with safe_dum
# Dynamically determine the path to the dlio_benchmark configurations
CONFIGS_DIR = Path(dlio_benchmark.__file__).resolve().parent / "configs" / "workload"


def find_workload_configs(config_dir):
    """Find all workload configuration files in the given directory."""
    logging.info(f"Searching for workload configuration files in {config_dir}")
    config_files = [os.path.splitext(f.name)[0] for f in config_dir.glob("*.yaml")]
    logging.info(f"Found {len(config_files)} configuration files.")
    return config_files


def execute_dlio_benchmark_query(workload, args, key, datatype=str):
    """Execute the dlio_benchmark_query executable with the given workload, arguments, and query key.

    Parameters:
        workload (str): The workload name.
        args (str): Additional arguments for the query.
        key (str): The query key.
        datatype (type): The expected datatype of the result. Defaults to str.

    Returns:
        The output of the query converted to the specified datatype.
    """
    query_command = f"dlio_benchmark_query workload={workload} {args} ++workload.workflow.query={key}"
    logging.info(f"Executing command: {query_command}")
    process = os.popen(query_command + " 2>/dev/null")
    output = process.read()
    exit_code = process.close()
    if exit_code is not None:
        logging.error(f"Command failed with exit code {exit_code}: {query_command}")
        raise RuntimeError(
            f"Failed to execute dlio_benchmark_query with command: {query_command}"
        )

    logging.info(f"Command executed successfully. Output: {output.strip()}")
    try:
        result = datatype(output)
        logging.info(f"Converted output to {datatype.__name__}: {result}")
        return result
    except ValueError as e:
        logging.error(f"Failed to convert output to {datatype.__name__}: {e}")
        raise ValueError(f"Failed to convert output to {datatype}: {e}")


def create_flux_execution_command(nodes=None, tasks_per_node=None):
    """Create a Flux execution command based on environment variables or input arguments."""
    if tasks_per_node is None:
        logging.error(
            "The 'tasks_per_node' argument is mandatory and must be provided."
        )
        raise ValueError(
            "The 'tasks_per_node' argument is mandatory and must be provided."
        )

    nodes = nodes or os.getenv("NODES")
    queue = os.getenv("QUEUE")
    WALLTIME = os.getenv("WALLTIME")

    if not all([nodes, queue, WALLTIME]):
        logging.error(
            "Environment variables 'NODES', 'QUEUE', and 'WALLTIME' must be set, unless overridden by input arguments."
        )
        raise EnvironmentError(
            "Environment variables 'NODES', 'QUEUE', and 'WALLTIME' must be set, unless overridden by input arguments."
        )

    command = f"flux run -N {nodes} --tasks-per-node={tasks_per_node} -q {queue} -t {WALLTIME} --exclusive"
    logging.info(f"Generated Flux execution command: {command}")
    return command


def generate_gitlab_ci_yaml(config_files):
    """Generate a GitLab CI YAML configuration with updated stages per workload."""
    system_name = os.getenv("SYSTEM_NAME")
    ci_config = {
        "variables": {},
        "stages": [
            "generate_data",
            "train",
            "compress_output",
            "move",
            "compact",
            "compress_final",
            "cleanup",
        ],
        "include": [
            {"project": "lc-templates/id_tokens", "file": "id_tokens.yml"},
            {"local": ".gitlab/scripts/common.yml"},
        ],
    }
    logging.info("Initialized CI configuration with default stages and variables.")

    # Gather and validate required environment variables
    env_vars = {
        "DATA_PATH": os.getenv("DATA_PATH"),
        "LOG_STORE_DIR": os.getenv("LOG_STORE_DIR"),
        "CUSTOM_CI_OUTPUT_DIR": os.getenv("CUSTOM_CI_OUTPUT_DIR"),
        "SYSTEM_NAME": os.getenv("SYSTEM_NAME", "default_system"),
    }

    for var_name, var_value in env_vars.items():
        if (
            not var_value and var_name != "SYSTEM_NAME"
        ):  # SYSTEM_NAME has a default value
            logging.error(f"Environment variable '{var_name}' is not set.")
            raise EnvironmentError(f"Environment variable '{var_name}' is not set.")
        logging.info(f"Environment variable '{var_name}' is set to '{var_value}'.")

    # Assign validated environment variables to local variables
    data_path = env_vars["DATA_PATH"]
    log_store_dir = env_vars["LOG_STORE_DIR"]
    custom_ci_output_dir = env_vars["CUSTOM_CI_OUTPUT_DIR"]
    system_name = env_vars["SYSTEM_NAME"]

    logging.info(f"Using DATA_PATH: {data_path}")
    logging.info(f"Using LOG_STORE_DIR: {log_store_dir}")
    logging.info(f"Using CUSTOM_CI_OUTPUT_DIR: {custom_ci_output_dir}")
    logging.info(f"Using SYSTEM_NAME: {system_name}")

    # Get dftracer version
    dftracer_version = dftracer.__version__
    logging.info(f"Detected dftracer version: {dftracer_version}")

    # Create log_dir variable
    log_dir = f"{log_store_dir}/v{dftracer_version}/{system_name}"
    logging.info(f"Generated log directory path: {log_dir}")

    # Generate a unique 8-digit UID for the run
    unique_run_id = datetime.now().strftime("%Y%m%d%H%M%S")
    logging.info(f"Generated unique run ID: {unique_run_id}")
    for idx, workload in enumerate(
        tqdm([config_files[:1]], desc="Processing workloads"), start=1
    ):
        workload_args = f"++workload.train.epochs=1"
        tp_size = execute_dlio_benchmark_query(
            workload, workload_args, "model.parallelism.tensor", int
        )
        pp_size = execute_dlio_benchmark_query(
            workload, workload_args, "model.parallelism.pipeline", int
        )
        samples_per_file = execute_dlio_benchmark_query(
            workload, workload_args, "dataset.num_samples_per_file", int
        )
        num_files = execute_dlio_benchmark_query(
            workload, workload_args, "dataset.num_files_train", int
        )
        batch_size = execute_dlio_benchmark_query(
            workload, workload_args, "reader.batch_size", int
        )
        nodes = int(os.getenv("NODES", 1))
        gpus = int(os.getenv("GPUS", 1))
        cores = int(os.getenv("CORES", 1))

        min_steps = 10
        cal_max_nodes = max(
            1, int(samples_per_file * num_files / batch_size / gpus / min_steps)
        )

        ranks = nodes * gpus
        tp_pp_product = tp_size * pp_size
        if ranks % tp_pp_product != 0:
            ranks = (ranks // tp_pp_product + 1) * tp_pp_product

        max_nodes = int(os.getenv("MAX_NODES", 1))
        nodes = max(1, ranks // gpus)
        if nodes > max_nodes:
            continue

        if max_nodes > cal_max_nodes:
            max_nodes = cal_max_nodes

        flux_cores_args = create_flux_execution_command(nodes, cores)
        flux_gpu_args = create_flux_execution_command(nodes, gpus)
        output = f"{custom_ci_output_dir}/{workload}/{nodes}/{unique_run_id}"
        dlio_data_dir = f"{data_path}/{workload}-{idx}-{nodes}/"
        workload_args = f"++workload.dataset.data_folder={dlio_data_dir}/data ++workload.train.epochs=1"
        generate_job_name = f"{workload}_{idx}_generate_data"
        ci_config[f"{generate_job_name}"] = {
            "stage": "generate_data",
            "extends": f".{system_name}",
            "script": [
                "ls",
                "source .gitlab/scripts/variables.sh",
                "source .gitlab/scripts/pre.sh",
                "which python; which dlio_benchmark;",
                f"if [ -d {dlio_data_dir} ]; then echo 'Directory {dlio_data_dir} already exists. Skipping data generation.'; else {flux_cores_args} dlio_benchmark workload={workload} {workload_args} ++workload.output.folder={output}/generate ++workload.workflow.generate_data=True ++workload.workflow.train=False; fi",
                f"if [ -d {dlio_data_dir} ] && grep -i 'error' {output}/generate/dlio.log; then echo 'Error found in dlio.log'; exit 1; fi",
            ],
        }

        while nodes <= max_nodes:
            output = f"{custom_ci_output_dir}/{workload}/{nodes}/{unique_run_id}"
            dlio_checkpoint_dir = f"{data_path}/{workload}-{idx}-{nodes}/"
            workload_args = f"++workload.dataset.data_folder={dlio_data_dir}/data ++workload.checkpoint.checkpoint_folder={dlio_checkpoint_dir}/checkpoint ++workload.train.epochs=1"
            base_job_name = f"{workload}_{idx}_{nodes}"
            flux_cores_args = create_flux_execution_command(nodes, cores)
            flux_gpu_args = create_flux_execution_command(nodes, gpus)

            for sub_step, stage in enumerate(
                [
                    "generate_data",
                    "train",
                    "compress_output",
                    "move",
                    "compact",
                    "compress_final",
                    "cleanup",
                ],
                start=1,
            ):
                tqdm.write(
                    f"Sub-step {sub_step}: Adding {stage} stage for workload '{workload}' with nodes {nodes}"
                )
                if stage == "train":
                    ci_config[f"{base_job_name}_train"] = {
                        "stage": "train",
                        "extends": f".{system_name}",
                        "script": [
                            "source .gitlab/scripts/variables.sh",
                            "source .gitlab/scripts/pre.sh",
                            "which python; which dlio_benchmark;",
                            f"{flux_gpu_args} dlio_benchmark workload={workload} {workload_args} ++workload.output.folder={output}/train hydra.run.dir={output}/train ++workload.workflow.generate_data=False ++workload.workflow.train=True",
                            f"if grep -i 'error' {output}/train/dlio.log; then echo 'Error found in dlio.log'; exit 1; fi",
                        ],
                        "needs": [f"{generate_job_name}"],
                        "variables": {
                            "DFTRACER_ENABLE": "1",
                            "DFTRACER_INC_METADATA": "1",
                        },
                    }

                elif stage == "compress_output":
                    ci_config[f"{base_job_name}_compress_output"] = {
                        "stage": "compress_output",
                        "extends": f".{system_name}",
                        "script": [
                            "source .gitlab/scripts/variables.sh",
                            "source .gitlab/scripts/pre.sh",
                            "which python; which dftracer_pgzip;",
                            f"{flux_cores_args} dftracer_pgzip -d {output}/train",
                            f"if find {output}/train -type f -name '*.pfw' | grep -q .; then echo 'Uncompressed .pfw files found!'; exit 1; fi",
                            f"if ! find {output}/train -type f -name '*.pfw.gz' | grep -q .; then echo 'No compressed .pfw.gz files found!'; exit 1; fi",
                        ],
                        "needs": [f"{base_job_name}_train"],
                    }

                elif stage == "move":
                    ci_config[f"{base_job_name}_move"] = {
                        "stage": "move",
                        "extends": f".{system_name}",
                        "script": [
                            "source .gitlab/scripts/variables.sh",
                            "source .gitlab/scripts/pre.sh",
                            f"mkdir -p {log_dir}/{workload}/nodes-{nodes}/{unique_run_id}/RAW/",
                            f"mv {output}/train/*.pfw.gz {log_dir}/{workload}/nodes-{nodes}/{unique_run_id}/RAW/",
                            f"mv {output}/train/.hydra {log_dir}/{workload}/nodes-{nodes}/{unique_run_id}/",
                            f"mv {output}/train/dlio.log {log_dir}/{workload}/nodes-{nodes}/{unique_run_id}/",
                        ],
                        "needs": [f"{base_job_name}_compress_output"],
                    }

                elif stage == "compact":
                    ci_config[f"{base_job_name}_compact"] = {
                        "stage": "compact",
                        "extends": f".{system_name}",
                        "script": [
                            "source .gitlab/scripts/variables.sh",
                            "source .gitlab/scripts/pre.sh",
                            # "source .gitlab/scripts/build.sh",
                            "which python; which dftracer_split;",
                            f"cd {log_dir}/{workload}/nodes-{nodes}/{unique_run_id}",
                            f"dftracer_split -d $PWD/RAW -o $PWD/COMPACT -s 1024 -n {workload}",
                            f"if ! find $PWD/COMPACT -type f -name '*.pfw.gz' | grep -q .; then echo 'No compacted .pfw.gz files found!'; exit 1; fi",
                        ],
                        "needs": [f"{base_job_name}_move"],
                    }

                elif stage == "compress_final":
                    ci_config[f"{base_job_name}_compress_final"] = {
                        "stage": "compress_final",
                        "extends": f".{system_name}",
                        "script": [
                            "source .gitlab/scripts/variables.sh",
                            "source .gitlab/scripts/pre.sh",
                            f"cd {log_dir}/{workload}/nodes-{nodes}/{unique_run_id}",
                            f"tar -czf RAW.tar.gz RAW",
                            f"tar -czf COMPACT.tar.gz COMPACT",
                        ],
                        "needs": [f"{base_job_name}_compact"],
                    }

                elif stage == "cleanup":
                    ci_config[f"{base_job_name}_cleanup"] = {
                        "stage": "cleanup",
                        "extends": f".{system_name}",
                        "script": [
                            "module load mpifileutils",
                            f"{flux_cores_args} drm {output}",
                        ],
                        "needs": [f"{base_job_name}_compress_final"],
                    }
            nodes *= 2

    return ci_config


def main():
    parser = argparse.ArgumentParser(
        description="Generate GitLab CI YAML for DLIO workloads."
    )
    parser.add_argument(
        "--output",
        type=str,
        default="run_dlio_workload_test_ci.yaml",
        help="Path to the output GitLab CI YAML file. Defaults to 'run_dlio_workload_test_ci.yaml' in the current directory.",
    )
    parser.add_argument(
        "--log-level",
        type=str,
        default="WARNING",
        choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"],
        help="Set the logging level. Defaults to 'INFO'.",
    )
    args = parser.parse_args()
    output_ci_file = Path(args.output).resolve()

    # Set the logging level based on user input
    logging.getLogger().setLevel(args.log_level.upper())

    logging.info(f"Output CI file path resolved to: {output_ci_file}")

    # Ensure the configs directory exists
    if not CONFIGS_DIR.exists():
        logging.error(f"Configurations directory '{CONFIGS_DIR}' does not exist.")
        return

    logging.info(f"Configurations directory '{CONFIGS_DIR}' exists.")

    # Find all workload configuration files
    config_files = find_workload_configs(CONFIGS_DIR)
    if not config_files:
        logging.warning("No workload configuration files found.")
        return

    logging.info(f"Found {len(config_files)} workload configuration files.")

    # Generate the GitLab CI YAML content
    try:
        ci_yaml = generate_gitlab_ci_yaml(config_files)
        logging.info("GitLab CI YAML content generated successfully.")
    except Exception as e:
        logging.error(f"Failed to generate GitLab CI YAML: {e}")
        return

    # Write the generated YAML to a file
    try:
        with open(output_ci_file, "w") as f:
            yaml.dump(ci_yaml, f, sort_keys=False)
        logging.info(f"GitLab CI YAML written successfully to {output_ci_file}")
    except Exception as e:
        logging.error(f"Failed to write GitLab CI YAML to file: {e}")
        return


if __name__ == "__main__":
    main()
