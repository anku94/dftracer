#!/bin/bash

LOG_FILE="$PWD/build.log"

echo "Running build.sh on $(hostname)" | tee -a "$LOG_FILE"

# shellcheck source=/dev/null

export site=$(ls -d $CUSTOM_CI_ENV_DIR/$ENV_NAME/lib/python*/site-packages/ 2>>"$LOG_FILE")

echo "Remove preinstall version of dlio_benchmark" | tee -a "$LOG_FILE"
echo "Command: pip uninstall dlio_benchmark" | tee -a "$LOG_FILE"
set -x
pip uninstall -y dlio_benchmark >>"$LOG_FILE" 2>&1
set +x
if [ $? -ne 0 ]; then
    echo "Failed to uninstall dlio_benchmark. Check the log file: $LOG_FILE" | tee -a "$LOG_FILE"
    exit 1
fi

set -x
rm -rf $site/*dlio_benchmark* >>"$LOG_FILE" 2>&1
set +x
if [ $? -ne 0 ]; then
    echo "Failed to remove dlio_benchmark files. Check the log file: $LOG_FILE" | tee -a "$LOG_FILE"
    exit 1
fi

echo "Installing DLIO benchmark with url git+$DLIO_BENCHMARK_REPO@$DLIO_BENCHMARK_TAG" | tee -a "$LOG_FILE"
echo "Command: pip install --no-cache-dir git+$DLIO_BENCHMARK_REPO@$DLIO_BENCHMARK_TAG" | tee -a "$LOG_FILE"
set -x
pip install --no-cache-dir git+$DLIO_BENCHMARK_REPO@$DLIO_BENCHMARK_TAG >>"$LOG_FILE" 2>&1
set +x
if [ $? -ne 0 ]; then
    echo "Failed to install DLIO benchmark. Check the log file: $LOG_FILE" | tee -a "$LOG_FILE"
    exit 1
fi

echo "Remove preinstall version of dftracer" | tee -a "$LOG_FILE"
echo "Command: pip uninstall pydftracer" | tee -a "$LOG_FILE"
set -x
pip uninstall -y pydftracer >>"$LOG_FILE" 2>&1
set +x
if [ $? -ne 0 ]; then
    echo "Failed to uninstall pydftracer. Check the log file: $LOG_FILE" | tee -a "$LOG_FILE"
    exit 1
fi

set -x
rm -rf $site/*dftracer* >>"$LOG_FILE" 2>&1
set +x
if [ $? -ne 0 ]; then
    echo "Failed to remove dftracer files. Check the log file: $LOG_FILE" | tee -a "$LOG_FILE"
    exit 1
fi

echo "Installing DFTracer" | tee -a "$LOG_FILE"
echo "Command: pip install --no-cache-dir --force-reinstall git+${DFTRACER_REPO}@${CI_COMMIT_REF_NAME}" | tee -a "$LOG_FILE"
set -x
pip install --no-cache-dir --force-reinstall git+${DFTRACER_REPO}@${CI_COMMIT_REF_NAME} >>"$LOG_FILE" 2>&1
set +x
if [ $? -ne 0 ]; then
    echo "Failed to install DFTracer. Check the log file: $LOG_FILE" | tee -a "$LOG_FILE"
    exit 1
fi

python -c "import dftracer; import dftracer.logger; print(dftracer.__version__);"
export PATH=$site/dftracer/bin:$PATH

export DFTRACER_VERSION=$(python -c "import dftracer; print(dftracer.__version__)") || { echo "Failed to get DFTRACER_VERSION"; exit 1; }

pushd "$LOG_STORE_DIR" || { echo "Failed to change directory to $LOG_STORE_DIR"; exit 1; }

SYSTEM=$(hostname | sed 's/[0-9]//g') || { echo "Failed to determine SYSTEM"; exit 1; }

LFS_DIR=v$DFTRACER_VERSION/$SYSTEM

if test -d "$LFS_DIR"; then
    echo "Branch $LFS_DIR Exists"
else
    git clone "ssh://git@czgitlab.llnl.gov:7999/iopp/dftracer-traces.git" "$LFS_DIR" || { echo "Failed to clone repository"; exit 1; }
    cd "$LFS_DIR" || { echo "Failed to change directory to $LFS_DIR"; exit 1; }
    git checkout -b "$LFS_DIR" || { echo "Failed to create branch $LFS_DIR"; exit 1; }
    cp "$LOG_STORE_DIR/v1.0.5-develop/corona/.gitattributes" . || { echo "Failed to copy .gitattributes"; exit 1; }
    cp "$LOG_STORE_DIR/v1.0.5-develop/corona/.gitignore" . || { echo "Failed to copy .gitignore"; exit 1; }
    cp "$LOG_STORE_DIR/v1.0.5-develop/corona/prepare_traces.sh" . || { echo "Failed to copy prepare_traces.sh"; exit 1; }
    cp "$LOG_STORE_DIR/v1.0.5-develop/corona/README.md" . || { echo "Failed to copy README.md"; exit 1; }
    git commit -a -m "added initial files" || { echo "Failed to commit files"; exit 1; }
    git push origin "$LFS_DIR" || { echo "Failed to push branch $LFS_DIR"; exit 1; }
    cd - || { echo "Failed to return to previous directory"; exit 1; }
fi

popd || { echo "Failed to change directory from $LOG_STORE_DIR"; exit 1; }
