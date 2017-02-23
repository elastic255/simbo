// Agent writer in project simpleIntBotnet

/* Initial beliefs and rules */

/* Initial goals */

//!start.

/* Plans */

+write[source(Ag)] <- println("Agent writer received a msg from: ",Ag);
						recordFile(2).
//+write[source(Ag)] <- ?jcm__art("writer","gui",ArtId); printMsgAgent("Agent writer received a msg from: ",Ag)[artifact_id(ArtId)];
						//recordFile(2).

{ include("$jacamoJar/templates/common-cartago.asl") }
{ include("$jacamoJar/templates/common-moise.asl") }

// uncomment the include below to have a agent that always complies with its organization  
{ include("$jacamoJar/templates/org-obedient.asl") }