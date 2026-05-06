#!/bin/bash
set -euxo pipefail

rsync -vaP treedecomp.js treedecomp.wasm  index.html msoos.org:/var/www/treedecomp/
