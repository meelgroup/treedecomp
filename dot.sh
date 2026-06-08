#!/bin/bash
cat <<EOF > a.cnf
p cnf 7 8
1 2 3 0
2 3 4 0
3 4 5 0
-4 5 6 0
5 6 7 0
-1 3 5 0
2 4 6 0
-5 -6 7 0
EOF

./treedecomp --tdvis stuff.dot a.cnf
dot -Tpdf stuff.dot -o stuff.pdf
dot -Tpng stuff.dot -o stuff.png
okular stuff.pdf
