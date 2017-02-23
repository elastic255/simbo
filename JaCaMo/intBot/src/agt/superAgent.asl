// Agent sample_agent in project simpleIntBotnet

/* Initial beliefs and rules */

//execution :- 1.  //this variable is used in the loop during the botnet execution
//current_node :- 0.

/* Initial goals */

!have_a_botnet.

/* Plans to start the botnet */

+!have_a_botnet
	<- !setup;
		!manage.
	
-!have_a_botnet[error(E),error_msg(Msg),code(Cmd),code_src(Src),code_line(Line)]
   <- .print("Failed to build a botnet due to: ",Msg," (",E,"). Command: ",Cmd, " on ",Src,":", Line).


//start the botnet
+!setup
	<- 	.wait(300);
		.my_name(Me);
		.println("My name: ",Me);
		.println;
		.println("********CREATING THE BOTNET*********");	
		.println;
		.wait(2000); // wait participants introduction
		
		//checking the agents of the botnet
		//+bot_state(Id,checking);
		.findall(Name,introduction(participant,Name),LP);
		.println("Send the status to ",LP);
		.println;
		.println;
		.println("*****BOTNET STARTED*****");
		.println;
		.println.
		

		
	
//managing the botnet

+!manage
	<- 	.println("Getting the infected node....");
		.println;
		request("A");
		.wait(6000);
		.println;
		.println("Calling the recon Agent to scan the node");
		.println;
		.send(reconAgent,tell,scanNode("S"));
		.println.
		
	
	//+waiting[source(Ag)] <- println("Waiting the results from: ",Ag).
	
	//+waiting[source(Ag)] <- ?jcm__art("w1","a1",ArtId); printMsgAgent("Waiting the results from: ",Ag)[artifact_id(ArtId)].


{ include("$jacamoJar/templates/common-cartago.asl") }
{ include("$jacamoJar/templates/common-moise.asl") }

// uncomment the include below to have a agent that always complies with its organization  
{ include("$jacamoJar/templates/org-obedient.asl") }