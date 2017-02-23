// CArtAgO artifact code for project intBot

package simulator;

import cartago.*;
import omnet.Omnet;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Collection;
import java.util.Collections;

public class Control extends Artifact {
	void init(int initialValue) {
		defineObsProperty("count", initialValue);
	}
	
	///////////
	Omnet omnet = new Omnet(); 
	////////////
	
	
	
	
	int omnetid=1;
	
	
	//print messages
    @OPERATION void printMsg(String msg){
    	String agentName = this.getOpUserName();
    	ObsProperty prop = this.getObsProperty("numMsg");
    	prop.updateValue(prop.intValue()+1);
    	//display.addText("Message at "+System.currentTimeMillis()+" from "+agentName+": "+msg);
    	System.out.println("Message "+msg+" from"+agentName);
    	//display.updateNumMsgField(prop.intValue());
       }
    
    //print messages received from other agents
    @OPERATION void printMsgAgent(String msg, String ag){
    	//ObsProperty prop1 = this.getObsProperty("numMsg");
    	//prop1.updateValue(prop1.intValue()+1);
    	//display.addText("Message received from "+ag+": "+msg);
    	//display.updateNumMsgField(prop1.intValue());
    	System.out.println("Message from: "+ag+". "+msg);
    }

	
	//request the scan of the environment (RA agent ask to the omnet++)
    @OPERATION void request(String msg) throws IOException {
    	String agentName = this.getOpUserName();
    	
    	//Print in a file for the OMNET++ reading
    	FileWriter file = new FileWriter("D:\\desenvolvimento\\omnet\\omnetpp-5.0\\samples\\Jteste12\\simulations\\terceiro\\io\\controles\\ijacamo.fl");
    	PrintWriter store = new PrintWriter(file);
    	FileWriter file2 = new FileWriter("D:\\desenvolvimento\\omnet\\omnetpp-5.0\\samples\\Jteste12\\simulations\\terceiro\\io\\controles\\cjacamo.fl");
    	PrintWriter store2 = new PrintWriter(file2);    	
    	
    	store.printf("%s", msg);
    	store2.printf(String.valueOf(omnetid), msg);
    	omnetid++;
    	
    	file.close();
    	file2.close();
    	
    	
    }
    
    
	@OPERATION void getResults() throws IOException{
    	String agentName = this.getOpUserName();
    	
    	//reading the file (test)
    	FileReader read = new FileReader("D:\\desenvolvimento\\omnet\\omnetpp-5.0\\samples\\Jteste12\\simulations\\terceiro\\io\\controles\\ojacamo.fl");
    	BufferedReader bRead = new BufferedReader(read);
    	
    	//reading and printing in the sA's GUI  
    	
    	
    	System.out.println("Results of the scan request from "+agentName+":");
    	
    	String result = bRead.toString();
    	while(result != null){
    		System.out.printf("%s\n",result);
    		result = bRead.readLine();
    	}
    	
    	//display.addText("Message at " +System.currentTimeMillis()+" from "+agentName+": "+bRead.readLine());
    	bRead.close();
    	
    }
    
    @OPERATION void lookingFile() throws IOException { 
    		
    	//boolean test = false;
    	
    	//reading the file (test)
    	FileReader read = new FileReader("/home/danziger/Workspace/simpleIntBotnet/files/jacamFileRead.txt");
    	BufferedReader bRead = new BufferedReader(read);
    	
    	String value = bRead.readLine();
    	int num = Integer.parseInt(value);
    	
    	
    	//loop to force the agent wait it
    	do{
    		if(num == 2){
    			//test = true;
        		//reading and printing in the sA's GUI
    			System.out.println("File was modified " +System.currentTimeMillis()+" : "+num);
            	//display.addText("File was modified " +System.currentTimeMillis()+" : "+num);
            	break;
    		}
    	}while(num != 2);    	    	
    	bRead.close();
		//return test;
    }
    
    @OPERATION void insertCommandOmnet() { 
    	
    }
    
    @OPERATION void pushPullCommandOmnet() throws IOException { 
    	omnet.controlLoop();
    }
    
    
    
}

