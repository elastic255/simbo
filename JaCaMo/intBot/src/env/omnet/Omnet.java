package omnet;

import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.io.RandomAccessFile;
import java.nio.CharBuffer;
import java.util.List;

import javax.swing.JOptionPane;

public class Omnet {
	
	protected long id=1; 
	//protected FileWriter controlOut1;
	//protected FileReader controlIn;
	protected RandomAccessFile controlIn;
	protected FileWriter dataOut1;
	protected FileReader dataIn1;
	
	//protected PrintWriter controlOut;
	protected RandomAccessFile controlOut;
	protected PrintWriter dataOut;
	public Comando comando = new Comando();
	
	
	public Omnet(){
		//Abre os arquivos
		try{
			String Path = "D:\\desenvolvimento\\omnet\\omnetpp-5.0\\samples\\inet\\src\\inet\\applications\\felipe\\Simulation\\terceiro\\io\\controles\\";
			//controlOut1 = new FileWriter(Path+"cjacamo.fl");
			//controlIn = new FileReader(Path+"ccjacamo.fl");
			controlIn = new RandomAccessFile(Path+"ccjacamo.fl","r");
			dataOut1 = new FileWriter(Path+"ijacamo.fl");
			dataIn1 = new FileReader(Path+"ojacamo.fl");
			//controlOut = new PrintWriter(controlOut1);
			controlOut = new RandomAccessFile(Path+"cjacamo.fl","rw");
			dataOut = new PrintWriter(dataOut1);
			
			
			
			controlOut.writeBytes("0");
			
		}catch (IOException e) {
			Omnet.infoBox("Erro ao abrir arquivos: "+e.getMessage());
			System.out.println("Erro ao abrir arquivos: "+e.getMessage());
		}
	}
	
	public void next(){
		//Deposita token(id), informando que há novas operações a serem realizadas.

		try{
			controlOut.seek(0);
			controlOut.writeBytes(Long.toString(id));
		}catch(Exception e){e.printStackTrace();}
		id++;

	}
	
	public boolean controlLoop(){
		//Controla o ciclo de operações. É o "main" da classe. 
		//CharBuffer cbuf = CharBuffer.allocate(1024);

		String snum;
		long num;
		
		try{			
			while(true){
				//lê o arquivo de controle(input) e verifica se o token foi atualizado.
				//Se sim popula arquivo de saída de dados com comandos.

				snum = controlIn.readLine();
				//System.out.println(snum);
				num = Long.valueOf(snum).longValue();			
				System.out.println(Long.toString(num));
				if(num == id){
					executar(comando);
					next();
					//System.out.println("entrei e sai");
					controlIn.seek(0);
					return true;
				}

				controlIn.seek(0);
				//return true;
			}
		}catch(IOException e){
			System.out.println("Erro omnet::controlLoop: "+e.getMessage());
			return false;
		}
		
		//return false;
	}
	
	public void executar(Comando b){
		//Pega todas as instruções realizadas e coloca no arquivo de saída.
		//TODO: colocar algum controle atômico para que não seja possível adicionar comandos após o início da saída dos comandos.
		List<Chuck> lista = b.getListas();
		for(Chuck tmp : lista){
			dataOut.println(tmp.toString());
		}
		
	}
	
	public void reflesh(){
		try{
		//controlIn.reset();
		dataIn1.reset();		
		}catch(Exception e){
			e.printStackTrace();
			System.out.println("Erro omnet::reflesh: "+e.getMessage());
		}	
	}
	
    public static void infoBox(String infoMessage)
    //função útil que abre uma caixa de diálogo.
    {
        JOptionPane.showMessageDialog(null, infoMessage, "InfoBox: " + "Erro classe omnet", JOptionPane.INFORMATION_MESSAGE);
    }


}
