filename = 'D:\desenvolvimento\omnet\omnetpp-5.0\samples\Jteste12\simulations\terceiro\io\sniffer\new.txt';
A = importdata(filename);
B = A.textdata;
nomes = B(:,1);
%ips----------------------
text_dest = B(:,2);
text_origem = B(:,3);
dest = cellfun(@ip2int,text_dest);
origem = cellfun(@ip2int,text_origem);
ips = unique([dest,origem]);
%----------------------
pdest = A.data(:,1);
porigem = A.data(:,2);
tamanho = A.data(:,3);
tempo = A.data(:,4);
%C = categorical(origem(dest==ip2int('10.0.0.1')),ips,{'1','2','3','4','5','6','7','8','9','10','11'});
%pie(C)