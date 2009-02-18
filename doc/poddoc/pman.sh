rm -f man/* html/*
for i in pod/*.[1-8]
do
	eval `echo $i|sed -e 's/\(.*\/\)\(.*\)\.\(.*\)/dir=\1 name=\2 sect=\3/'`
	ln $i $name
	pod2man --section=$sect --release="GNUbatch Release 1" --center="GNUbatch Batch Scheduler" $name >man/$name.$sect
	pod2html --noindex --outfile=html/${name}_$sect.html --infile=$name
	rm -f $name
done
