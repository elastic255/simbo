import java.util.Scanner;

/* coding:utf-8
 * Copyright (C) dirlt
 */

public class JavaConsole{

     public static void main(String[] args){
		 System.out.println("Hellow World. Eu sou java.\n E vou reescrever os parâmetros passados pelo C++:");
                for (String s: args) {
            System.out.println(s);
                }
	//return 100;
     }

	public static int Bob(String[] args){
		 System.out.println("Hellow World. Eu sou java. Classe Bob\n E vou reescrever os parâmetros passados pelo C++:");
             Scanner reader = new Scanner(System.in);   
                for (String s: args) {
                System.out.println("Entre com um número");
                int n = reader.nextInt();
            	System.out.println(s);
            
                }
	return 100;
     }
	
	public static void console(){
		ConsoleInput consoleI = new ConsoleInput();
		Thread consoleInput = new Thread(consoleI);
		consoleInput.start();
		System.out.println("Hello");
		
	}
	
	public static void write(String arg){
		System.out.println(arg);		
	}
	
	
}
