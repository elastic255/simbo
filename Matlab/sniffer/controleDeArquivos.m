fileC = fopen('D:\desenvolvimento\omnet\omnetpp-5.0\samples\Jteste12\simulations\terceiro\io\controles\cmatlab.fl','r+');
fileO = fopen('D:\desenvolvimento\omnet\omnetpp-5.0\samples\Jteste12\simulations\terceiro\io\controles\omatlab.fl','r+');

id = 0;
n=0;
 while n < 500
    frewind(fileC)
    a = fscanf(fileC,'%d');
    if id >= a
        continue;
    end
    id = a+1;
    
    c = rede;
    c.nos(1).origemPacotesPie(n);
    
    n = n+1;
    frewind(fileC)
    fprintf(fileC,'%d',id);
    frewind(fileC)
 end
fclose(fileC);