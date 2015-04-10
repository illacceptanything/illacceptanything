using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class Static : MonoBehaviour {

	Image image;
	public Color activeColor;
	Color targetColor;
	Color transparent = new Color(0, 0, 0, 0);
	
	void Start () {
		image = gameObject.GetComponent<UnityEngine.UI.Image>();	
		targetColor = image.color = transparent;	
		StartCoroutine(animate());
	}
	
	void Update () {
		image.color = Color.Lerp(image.color, targetColor, Time.deltaTime/0.9f);
		
	}
	
	public void flashOut(){	
		image.color = activeColor;
		targetColor = transparent;
	}
	
	IEnumerator animate(){
	
		image.rectTransform.localPosition = new Vector3(595, 500, 0);
		yield return new WaitForSeconds(0.03f);
		image.rectTransform.localPosition = new Vector3(575, 440, 0);
		yield return new WaitForSeconds(0.03f);
		image.rectTransform.localPosition = new Vector3(605, 330, 0);
		yield return new WaitForSeconds(0.03f);
		image.rectTransform.localPosition = new Vector3(580, 470, 0);
		yield return new WaitForSeconds(0.03f);
		image.rectTransform.localPosition = new Vector3(585, 390, 0);
		yield return new WaitForSeconds(0.03f);
		
		StartCoroutine(animate());

	}
		
		
}
