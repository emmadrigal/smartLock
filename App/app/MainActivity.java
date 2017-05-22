package com.example.mmontero.prueba;

import android.support.v7.app.AppCompatActivity;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.DataOutputStream;
import java.net.Socket;
import java.net.UnknownHostException;

import android.os.AsyncTask;
import android.os.Bundle;
import android.app.Activity;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class MainActivity extends Activity {

    TextView textResponse;
    Button buttonAbrir, buttonCerrar, buttonAgregar,buttonEliminar,buttonChequear;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        buttonAbrir = (Button)findViewById(R.id.abrir);
        buttonCerrar = (Button)findViewById(R.id.cerrar);
        buttonAgregar = (Button)findViewById(R.id.agregar);
        buttonEliminar = (Button)findViewById(R.id.eliminar);
        buttonChequear = (Button)findViewById(R.id.chequear);
        textResponse = (TextView)findViewById(R.id.response);

        buttonAbrir.setOnClickListener(buttonAbrirOnClickListener);
        buttonCerrar.setOnClickListener(buttonCerrarOnClickListener);
        buttonAgregar.setOnClickListener(buttonAgregarOnClickListener);
        buttonEliminar.setOnClickListener(buttonEliminarOnClickListener);
        buttonChequear.setOnClickListener(buttonChequearOnClickListener);

    }

    OnClickListener buttonAbrirOnClickListener =
            new OnClickListener(){

                @Override
                public void onClick(View arg0) {
                    MyClientTask myClientTask = new MyClientTask(
                            "192.168.4.1",
                            333,"0");
                    myClientTask.execute();
                }};

    OnClickListener buttonCerrarOnClickListener =
            new OnClickListener(){

                @Override
                public void onClick(View arg0) {
                    MyClientTask myClientTask = new MyClientTask(
                            "192.168.4.1",
                            333,"1");
                    myClientTask.execute();
                }};

    OnClickListener buttonAgregarOnClickListener =
            new OnClickListener(){

                @Override
                public void onClick(View arg0) {
                    MyClientTask myClientTask = new MyClientTask(
                            "192.168.4.1",
                            333,"2");
                    myClientTask.execute();
                }};

    OnClickListener buttonEliminarOnClickListener =
            new OnClickListener(){

                @Override
                public void onClick(View arg0) {
                    MyClientTask myClientTask = new MyClientTask(
                            "192.168.4.1",
                            333,"3");
                    myClientTask.execute();
                }};

    OnClickListener buttonChequearOnClickListener =
            new OnClickListener(){

                @Override
                public void onClick(View arg0) {
                    MyClientTask myClientTask = new MyClientTask(
                            "192.168.4.1",
                            333,"4");
                    myClientTask.execute();
                }};


    public class MyClientTask extends AsyncTask<Void, Void, Void> {

        String dstAddress;
        int dstPort;
        String response = "";

        MyClientTask(String addr, int port,String dato){
            dstAddress = addr;
            dstPort = port;
            response = dato;
        }

        @Override
        protected Void doInBackground(Void... arg0) {

            Socket socket = null;

            try {
                socket = new Socket(dstAddress, dstPort);

                DataOutputStream mensaje;
                mensaje = new DataOutputStream(socket.getOutputStream());
                mensaje.writeUTF(response);


            } catch (UnknownHostException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
                response = "UnknownHostException: " + e.toString();
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
                response = "IOException: " + e.toString();
            }finally{
                if(socket != null){
                    try {
                        socket.close();
                    } catch (IOException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                }
            }
            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            textResponse.setText(response);
            super.onPostExecute(result);
        }

    }

}
