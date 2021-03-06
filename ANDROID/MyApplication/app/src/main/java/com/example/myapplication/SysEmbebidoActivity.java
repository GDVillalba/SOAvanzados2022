package com.example.myapplication;
import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageButton;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

public class SysEmbebidoActivity extends Activity implements SysEmbebidoC.View{
    private ImageButton btnAumentarVel;
    private ImageButton btnAumentarTermo;
    private ImageButton btnDisminuirTermo;
    private static HandlerSysEmbebidoP p;
    private TextView txtV;
    private TextView txtT;
    private TextView txtTmp;
    private Switch off_on;
    private String address;
    private SensorProximidad sensor;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent intent=getIntent();
        Bundle extras=intent.getExtras();
        address= extras.getString(ConexionBTActivity.CODE_ADDRESS);
        sensor= new SensorProximidad(this);
        setContentView(R.layout.activity_mprincipal);
        this.btnAumentarTermo   = findViewById(R.id.btnAumentarT);
        this.btnDisminuirTermo  = findViewById(R.id.btnDisminuirT);
        this.btnAumentarVel     = findViewById(R.id.btnAumentarV);
        this.txtT               = findViewById(R.id.txtUmbral);
        this.txtV               = findViewById(R.id.txtVel);
        this.txtTmp             = findViewById(R.id.txtTempAmb);
        this.off_on             = findViewById(R.id.off_on);
        EngineSysEmbebidoM m    = new EngineSysEmbebidoM();
        this.p                  = new HandlerSysEmbebidoP(this,m);
        m.setPresenter(p);

        this.off_on.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                if(off_on.isChecked()){
                    p.btnEncender();
                }else{
                    p.btnApagar();
                }
            }
        });
        this.btnAumentarTermo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                p.btnAumentarT();
            }
        });
        this.btnDisminuirTermo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                p.btnDisminuirT();
            }
        });
        this.btnAumentarVel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                p.btnAumentarV();
            }
        });
    }

    public void receiveSensor(){
        p.btnAumentarV();
    }
    @Override
    protected void onResume() {
        super.onResume();
        if(!p.isBTConnectado()) p.conectarBT(address);
    }
    @Override
    public SensorManager getSensorManager( ){
        return (SensorManager) getSystemService(SENSOR_SERVICE);
    }
    @Override
    public void reposar(){
        off_on.setChecked(false);
        //Oculta el texto de velocidad y termostato
        txtT.setTextColor( Color.argb(0,255,255,255));
        txtV.setTextColor( Color.argb(0,255,255,255));
        p.reposarSys();
    }
    @Override
    public void activar() {
        off_on.setChecked(true);
        //Hace visible el texto de velocidad y termostato
        txtT.setTextColor( Color.argb(255,255,255,255));
        txtV.setTextColor( Color.argb(255,255,255,255));
        p.activarSys();
    }
    @Override
    public void mostrarVel(String str){
        this.txtV.setText(str);
    }
    @Override
    public void mostrarUmbralTermo(String str){
        this.txtT.setText(str);
    }
    @Override
    public void mostrarTempAmbiente(String str){
        this.txtTmp.setText(str);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        p.onDestroy();
        sensor.Parar_Sensores();
    }
    @Override
    public void showToast(String message) {
        Toast.makeText(getApplicationContext(), message, Toast.LENGTH_SHORT).show();
    }
}