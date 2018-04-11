PIBIC=/home/fabio/Documentos/PIBIC
for i in `seq 0 99`;
do
	shuf $PIBIC/paths > $PIBIC/paths2
	rm $PIBIC/paths
	mv $PIBIC/paths2 $PIBIC/paths
	(/usr/bin/time -v opp_run -m -n ../../../../../../examples:../../../../../../showcases:../../../../..:../../../../../../tutorials --image-path=../../../../../../images -l ../../../../../INET omnetpp.ini --seed-set=$i -c Athena_10 -u Cmdenv) &>> Athena_10.txt
        shuf $PIBIC/paths > $PIBIC/paths2
        rm $PIBIC/paths
        mv $PIBIC/paths2 $PIBIC/paths
	(/usr/bin/time -v opp_run -m -n ../../../../../../examples:../../../../../../showcases:../../../../..:../../../../../../tutorials --image-path=../../../../../../images -l ../../../../../INET omnetpp.ini --seed-set=$i -c Athena_100 -u Cmdenv) &>> Athena_100.txt
        shuf $PIBIC/paths > $PIBIC/paths2
        rm $PIBIC/paths
        mv $PIBIC/paths2 $PIBIC/paths
	(/usr/bin/time -v opp_run -m -n ../../../../../../examples:../../../../../../showcases:../../../../..:../../../../../../tutorials --image-path=../../../../../../images -l ../../../../../INET omnetpp.ini --seed-set=$i -c Athena_1000 -u Cmdenv) &>> Athena_1000.txt
done
