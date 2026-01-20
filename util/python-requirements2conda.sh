#!/usr/bin/env bash

# Header for the environment file
cat <<EOF > environment.yml
name: heepokranios
channels:
  - defaults
dependencies:
  - python=3.8
  - cmake>=3.10
  - pip
  - pip:
EOF

# Loop through all arguments (the requirements files)
for req_file in "$@"; do
  if [ -f "$req_file" ]; then
    # Clean lines (remove comments and empty lines) and append
    sed '/^$/d; /^#/d' "$req_file" | while read -r line; do
      echo "    - $line" >> environment.yml
    done
  fi
done