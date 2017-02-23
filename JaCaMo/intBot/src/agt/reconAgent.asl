{ include("$jacamoJar/templates/common-cartago.asl") }
{ include("$jacamoJar/templates/common-moise.asl") }
//{ include("$jacamoJar/templates/org-obedient.asl") }

// Agent ra in project autonomousBotnet

/* Initial beliefs and rules */

//plays(initiator,superAgent).

/* Initial goals */

//lookingFile.

//receive(msg).
plays(initiator,superAgent).
//pos(node).
//scan(nodes).

//!start.

/* Plans */

//send a message to the super agent introducing myself as a participant
+plays(initiator,In)
	: .my_name(Me)
	<- .send(In,tell,introduction(participant,Me)).

//+start_Scanning[source(Ag)] <- ?jcm__art("w1","a1",ArtId); printMsgAgent("I received the msg!",Ag)[artifact_id(ArtId)].
//+start_Scanning[source(Ag)] <- println("I received a message from ",Ag).
	
+scanNode(Node)  
	<- 	.println("Starting the scan...",Node);
		request(Node);
		println;
		.wait(6000);
		.println("Reading the results....");
		.println;
		getResults;  //call the operation in the Control artifact
		.println;
		.println;
		.println("Finished this first part....").



/*+pos(X1) : scan(nodes)
	    <- 	!scanEnvironment(nodes);
	     	!writeToken;
	     	!readToken.
	    	
+!scanEnvironment(nodes) : true
		<- scanEnv(nodes).
		
+!writeToken : true <- recordFile(1).

+!readToken : true 
	<- 	.send(superAgent,tell,waiting);
		.wait(500);
		.send(writer,tell,write);
		.wait(500);
		lookingFile.
		
		*/
			
			









 


 
