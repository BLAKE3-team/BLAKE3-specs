#! /usr/bin/env bash

set -e -u -o pipefail

cd "$(dirname "$BASH_SOURCE")"

docker build --tag blake3_specs_image .

docker run --name blake3_specs_container blake3_specs_image

docker cp blake3_specs_container:/src/build/blake3.pdf .

docker rm blake3_specs_container
