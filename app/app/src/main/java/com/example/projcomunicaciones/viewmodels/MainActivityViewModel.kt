package com.example.projcomunicaciones.viewmodels
import android.util.Log
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.projcomunicaciones.models.Command
import com.example.projcomunicaciones.models.Coords
import com.example.projcomunicaciones.models.EspData
import com.example.projcomunicaciones.repositories.MQTTRepository
import com.google.gson.Gson
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import org.osmdroid.util.GeoPoint

class   MainActivityViewModel:ViewModel() {
    private val serverUri = "tcp://test.mosquitto.org:1883"
    val mqtt= MQTTRepository(serverUri,this)
    private val _espLiveLocation = MutableLiveData<EspData>()
    private val _espLiveLogs=MutableLiveData<String>()
    private val _connectedStatus=MutableLiveData<Boolean>()
    val espLiveLocation: LiveData<EspData> get() = _espLiveLocation
    val connectedStatus: LiveData<Boolean> get() = _connectedStatus
    val espLiveLogs: LiveData<String> get()=_espLiveLogs
    fun updateLocation(espData: EspData) {
        _espLiveLocation.postValue(espData)
    }
    fun updateLogs(logs:String){
        _espLiveLogs.postValue(logs)
    }
     fun connectMQTT(){
        viewModelScope.launch(Dispatchers.IO){
            if(mqtt.mqttConnect())
                _connectedStatus.postValue(true)
        }
    }
    fun sendLocationMQTT(location:GeoPoint?,topic:String){
        val lat=location!!.latitude
        val lon=location!!.longitude
        val coords=Coords(lat,lon)
        val data=Command("translation",coords)
        val json= Gson().toJson(data)
        pubMQTT(json,topic)
    }
    fun sendOrientationMQTT(orientation:Float,topic: String){
        val data= Command("orientation",orientation)
        val json=Gson().toJson(data)
        pubMQTT(json,topic)
    }
    private fun pubMQTT(data:String,topic:String){
        Log.d("MQTT", "Publishing to MQTT...")
        viewModelScope.launch(Dispatchers.IO){
            if(mqtt.publishMessage(topic,data))
                Log.i("MQTT","Message Published~")
        }
    }
}