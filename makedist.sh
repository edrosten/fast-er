rm -r CVS 
doxygen
cd latex
make
cp refman.pdf ..
cd ..
cd fast_trees
rm -r CVS
tar -xvjf FAST_trees.tar.bz2
mv FAST_trees/* .
rm -r FAST_trees.tar.bz2 FAST_trees
cd ..
rm makedist.sh
