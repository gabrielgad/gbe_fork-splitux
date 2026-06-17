#!/usr/bin/env bash


if [ "$(id -u)" -ne 0 ]; then
  echo "Please run as root" >&2
  exit 1
fi

retry() {
  local max_attempts=$1; shift
  local delay=$1; shift
  local attempt=1
  while true; do
    "$@" && return 0
    if (( attempt >= max_attempts )); then
      echo "Failed after $max_attempts attempts: $*" >&2
      return 1
    fi
    echo "Attempt $attempt/$max_attempts failed. Retrying in ${delay}s..."
    sleep "$delay"
    ((attempt++))
  done
}

python_package="python3.12"
venv=".env-linux"
reqs_file="requirements.txt"
script_dir=$( cd -- "$( dirname -- "${0}" )" &> /dev/null && pwd )

retry 3 10 apt update -y || exit 1
apt install software-properties-common -y
retry 3 10 add-apt-repository ppa:deadsnakes/ppa -y
retry 3 10 apt update -y || exit 1
retry 3 10 apt install "$python_package" -y || exit 1
apt install "$python_package-dev" -y || exit 1
apt install "$python_package-venv" -y || exit 1
apt install python3-dev -y || exit 1

[[ -d "$script_dir/$venv" ]] && rm -r -f "$script_dir/$venv"

$python_package -m venv "$script_dir/$venv" || exit 1
sleep 1

chmod 777 "$script_dir/$venv/bin/activate"
source "$script_dir/$venv/bin/activate"

pip install -r "$script_dir/$reqs_file"
exit_code=$?

deactivate
exit $exit_code
