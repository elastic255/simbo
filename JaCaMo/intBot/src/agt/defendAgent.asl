// Agent defendAgent in project intBot

/* Initial beliefs and rules */

plays(initiator,superAgent).

/* Initial goals */

/* Plans */

+plays(initiator,In)
	: .my_name(Me)
	<- .send(In,tell,introduction(participant,Me)).

{ include("$jacamoJar/templates/common-cartago.asl") }
{ include("$jacamoJar/templates/common-moise.asl") }

// uncomment the include below to have a agent that always complies with its organization  
{ include("$jacamoJar/templates/org-obedient.asl") }
