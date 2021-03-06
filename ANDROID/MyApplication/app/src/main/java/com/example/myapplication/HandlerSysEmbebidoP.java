package com.example.myapplication;

public class HandlerSysEmbebidoP implements SysEmbebidoC.Presenter, SysEmbebidoC.Model.OnEventListener{

    private SysEmbebidoC.View mainView;
    private SysEmbebidoC.Model model;

    public HandlerSysEmbebidoP(SysEmbebidoC.View mainView, SysEmbebidoC.Model model){
        this.mainView = mainView;
        this.model = model;
    }

    @Override
    public boolean isBTConnectado() {
        return this.model.isBTConnectado(this);
    }

    @Override
    public void btnAumentarT() {
        model.aumentarUmbralTermo(this);
    }

    @Override
    public void btnDisminuirT() {
        model.disminuirUmbralTermo(this);
    }

    @Override
    public void btnAumentarV() {
        model.aumentarVel(this);
    }

    @Override
    public void btnEncender() { model.encender(this); }

    @Override
    public void btnApagar() { model.apagar(this); }

    @Override
    public void iniciarSys() { model.inicializarValores(this); }

    @Override
    public void activarSys() { model.encenderSys(this); }

    @Override
    public void reposarSys() { model.apagarSys(this); }

    @Override
    public void onDestroy() {
        this.model.onDestroy(this);
        mainView = null;
    }

    @Override
    public boolean conectarBT(String address) {
        return this.model.conectarBT(this,address);
    }

    @Override
    public void onEventReposar(){
        if(mainView != null){
            mainView.reposar();
        }
    }

    @Override
    public void onEventActivar(){
        if(mainView != null){
            mainView.activar();
            //this.iniciarSys();
        }

    }

    @Override
    public void onEventVel(String string){
        if(mainView != null){
            mainView.mostrarVel(string);
        }
    }

    @Override
    public void onEventTempAmbiente(String string) {
        if(mainView != null){
            mainView.mostrarTempAmbiente(string);
        }
    }
    @Override
     public void onEventUmbral(String string) {
        if(mainView != null){
            mainView.mostrarUmbralTermo(string);
        }
    }

    @Override
    public void alert(String s) {
        if(mainView != null){
            this.mainView.showToast(s);
        }
    }
}