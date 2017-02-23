package omnet;

public class Chuck {
	//classe que guarda a operação a ser realizada pelo omnet++.
	
	private long id;
	private String op;
	private static long globalId = 0;
	
	Chuck(){id=-1;}
	
	public String toString(){
		return op;
	}
	
	public void setCommand(String a){
		op = juntaId(a);
	}
	
	private String juntaId(String a){
		id = Chuck.getNextId();
		a.concat(" #");
		return a.concat(String.valueOf(id));
	}
	
	static private long getNextId(){
		Chuck.globalId++;
		return Chuck.globalId++;
	}
}
