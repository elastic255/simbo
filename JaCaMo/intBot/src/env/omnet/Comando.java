package omnet;

import java.util.ArrayList;
import java.util.List;

public class Comando {
	
	private List<Chuck> lista = new ArrayList<Chuck>();
	
	
	public CScan Scan = new CScan();
	public CSys Sys = new CSys();
	public CAttack Attack = new CAttack();
	public CDefense Defense = new CDefense();
	
	public List<Chuck> getListas(){
		lista = Scan.getLista();
		lista.addAll(Sys.getLista());
		lista.addAll(Attack.getLista());
		lista.addAll(Defense.getLista());
		return lista;};
	


}
