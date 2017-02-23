package omnet;

import java.util.ArrayList;
import java.util.List;

class CSys {
	
	private List<Chuck> lista = new ArrayList<Chuck>();
	
	public List<Chuck> getLista(){return lista;};
	public void getIdHostBotMaster(){
		Chuck chuck = new Chuck();
		chuck.setCommand("IBOTMASTER");
		
	};
	
}
