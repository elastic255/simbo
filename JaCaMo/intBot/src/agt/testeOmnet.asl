// Agent testeOmnet in project intBot

/* Initial beliefs and rules */

/* Initial goals */

!start.

/* Plans */

+!start : true <- 	.print("inicio");
					.send(testeOmnet,tell,loop(1)).
					
+loop(N)  <- .print("hello world.");
		  pushPullCommandOmnet;
		  .wait(1000);
		  .send(testeOmnet,tell,loop(N+1)).
				

{ include("$jacamoJar/templates/common-cartago.asl") }
{ include("$jacamoJar/templates/common-moise.asl") }

// uncomment the include below to have a agent that always complies with its organization  
//{ include("$jacamoJar/templates/org-obedient.asl") }
