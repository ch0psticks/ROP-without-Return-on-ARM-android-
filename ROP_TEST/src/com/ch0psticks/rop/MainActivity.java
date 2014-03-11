package com.ch0psticks.rop;

import android.os.Bundle;
import android.app.Activity;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;
import android.view.View.OnClickListener;

public class MainActivity extends Activity {

	Button BtnExecRop, BtnTestCommand;
	EditText EdtLogs;
	
	static String TAG = "ROP_TEST";
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		BtnExecRop = (Button)findViewById(R.id.btnExecRop);
		BtnTestCommand = (Button)findViewById(R.id.BtnTest_command);
		
		EdtLogs = (EditText)findViewById(R.id.edtLogs);
		
		BtnExecRop.setOnClickListener(new OnClickListener(){
			public void onClick(View v){
				Toast.makeText(MainActivity.this, "Start Execute ROP chain",
						Toast.LENGTH_LONG).show();
				RopExec();
			}
		});
		
		BtnTestCommand.setOnClickListener( new OnClickListener(){
			public void onClick(View v){
				String strTestCommandResult = TestCommand("am start -a android.intent.action.MAIN -c android.intent.category.TEST -n com.android.term/.Term");
		
				//use " cat /mnt/sdcard/123.txt" to verify the execute result!
				//expected outout: Sat Mar  8 16:14:47 GMT 2014
				//String strTestCommandResult = TestCommand("date > /mnt/sdcard/123.txt");
				Toast.makeText(MainActivity.this, strTestCommandResult, Toast.LENGTH_SHORT).show();
				
			}
		});
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
	
	
	public void RopExec(){
		Log.i(TAG, "BEFORE CALL NativeCode");
		String strFromNativeCode = ExecRop("hello", 12);
		Toast.makeText(MainActivity.this, strFromNativeCode, Toast.LENGTH_SHORT).show();
		Log.i(TAG, "After CALL NativeCode");
	}
	
	static{
		System.loadLibrary("ROP_TEST");
	}
	//declaration 
	public native String ExecRop(String strTest,int count);
	public native String TestCommand(String strCommand);
}

