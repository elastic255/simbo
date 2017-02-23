import java.util.Scanner;

public class ConsoleInput implements Runnable{
	
	Scanner reader = new Scanner(System.in);  
	int escolha;
	
	public void run () {
		while(true){
		  System.out.println("Entre com a instruções");
		  String entrada = reader.next();
		  if(entrada.contains("teste")){escolha = 1; System.out.println("teste true!!!");}
		}

	}
}
