cd D:\desenvolvimento\omnet\omnetpp-5.0\samples\inet\src\inet\applications\simbo\Simulation\Quinto;
%[x, y] = dos('D:\desenvolvimento\omnet\omnetpp-5.0\samples\inet\src\inet\applications\simbo\Simulation\Quinto\run_bala-Console-ini3.bat > teste.txt &');
[x, y] = dos('D:\desenvolvimento\omnet\omnetpp-5.0\samples\inet\src\inet\applications\simbo\Simulation\Quinto\run_bala-Console-ini3.bat && exit &');
% precisa rodar em background, se não trava na tela Dos.

fileCC = fopen('D:\desenvolvimento\omnet\omnetpp-5.0\samples\inet\src\inet\applications\simbo\Simulation\Quinto\io\controles\ccmatlab.fl','r+');
fileO = fopen('D:\desenvolvimento\omnet\omnetpp-5.0\samples\inet\src\inet\applications\simbo\Simulation\Quinto\io\controles\omatlab.fl','r+');
fileC = fopen('D:\desenvolvimento\omnet\omnetpp-5.0\samples\inet\src\inet\applications\simbo\Simulation\Quinto\io\controles\cmatlab.fl','r+');
%fileI = fopen('D:\desenvolvimento\omnet\omnetpp-5.0\samples\inet\src\inet\applications\simbo\Simulation\Quinto\io\controles\imatlab.fl','r+');

id = 0;
n=0;
 while n < 500
    frewind(fileCC);
    
    a = fscanf(fileCC,'%d');
    if id >= a
        continue;
    end
    
    
    frewind(fileC);
    frewind(fileO);
    
    fileI = fopen('D:\desenvolvimento\omnet\omnetpp-5.0\samples\inet\src\inet\applications\simbo\Simulation\Quinto\io\controles\imatlab.fl','w+');
    id = a;
    
    %c = rede;
    %c.nos(1).origemPacotesPie(n);
    %x = input('Comando:');
    
    n = n+1;
    fprintf(fileI,'%s','');
    fprintf(fileC,'%d',id);
    
    frewind(fileO);
    frewind(fileI);
    frewind(fileC);
    
    
    fclose(fileI);
 end
 
fclose(fileCC);
fclose(fileC);
fclose(fileO);