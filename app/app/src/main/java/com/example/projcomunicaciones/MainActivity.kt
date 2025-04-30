package com.example.projcomunicaciones

import android.animation.ValueAnimator
import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.viewModels
import androidx.lifecycle.Observer
import com.example.projcomunicaciones.viewmodels.MainActivityViewModel
import org.osmdroid.config.Configuration
import org.osmdroid.config.IConfigurationProvider
import org.osmdroid.events.MapEventsReceiver
import org.osmdroid.util.GeoPoint
import org.osmdroid.views.MapController
import org.osmdroid.views.MapView
import org.osmdroid.views.overlay.MapEventsOverlay
import org.osmdroid.views.overlay.Marker
import org.osmdroid.views.overlay.Overlay
import org.osmdroid.views.overlay.OverlayManager

class MainActivity : ComponentActivity() {
    lateinit var publishBtn:Button
    lateinit var mapView:MapView
    lateinit var mapController: MapController
    lateinit var mapOverlays:OverlayManager
    var selectedLocation:GeoPoint?=null
    val cucutaGeopoint=GeoPoint(7.889325709440754, -72.49674315409291)
    private val vm: MainActivityViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        publishBtn=findViewById(R.id.btPublish)
        loadMap(18,cucutaGeopoint)
        vm.connectMQTT()
    }

    override fun onStart() {
        super.onStart()
        publishBtn.setOnClickListener{
            selectedLocation?.let {
                vm.sendMQTT(selectedLocation)
            }
        }
        vm.espLiveData.observe(this, Observer{espData->
            espData?.let {
                addPositionMarker(it.fetchLat(),it.fetchLong(),it.fetchSpeed())
            }
        })
        vm.connectedStatus.observe(this,{connected->
            val connectedTv=findViewById<TextView>(R.id.tvStatusMQTT)
            if(connected)
                connectedTv.setText("Conectado")
            else {
                connectedTv.setText("Desconectado")
            }
        })
    }
    private fun loadMap(zoom:Int,center:GeoPoint){
        mapView=findViewById(R.id.mvMapView)
        mapView.setMultiTouchControls(true)
        mapOverlays=mapView.overlayManager
        val config: IConfigurationProvider = Configuration.getInstance()
        val context = applicationContext
        config.userAgentValue = packageName
        config.load(context, getSharedPreferences("osmdroid", MODE_PRIVATE))
        mapController=MapController(mapView)
        mapController.apply {
            setCenter(center)
            setZoom(zoom)
        }
        val mapEventsReceiver=object:MapEventsReceiver{
            override fun singleTapConfirmedHelper(p: GeoPoint?): Boolean {
                p?.let{
                    val lat=it.latitude
                    val lon=it.longitude
                    selectedLocation=GeoPoint(lat,lon)
                    val selectedMarker=Marker(mapView)
                    val markerId="selectedLocation"
                    selectedMarker.apply {
                        position=selectedLocation
                        title="Ubicación Seleccionada"
                        snippet="${String.format("%.4f",lat)},${String.format("%.4f",lon)}"
                        id=markerId
                    }
                    if(!markerExist(markerId))
                        mapOverlays.add(selectedMarker)
                    else
                        updatePositionMarker(lat,lon,markerId,0.0f,false)
                }
                mapView.invalidate()
                return true
            }

            override fun longPressHelper(p: GeoPoint?): Boolean {
                return false
            }

        }
        val overlayEventos = MapEventsOverlay(mapEventsReceiver)
        mapView.overlays.add(overlayEventos)
    }
    private fun addPositionMarker(lat:Double,long: Double,speed:Float){
    val markerId="esp32"
    val newPos=GeoPoint(lat,long)
    if(!markerExist(markerId)){
        val espMarker=Marker(mapView)
        espMarker.apply {
            position=newPos
            id= markerId
            title="Posición del GPS"
            snippet="${speed} Km/h"
            mapOverlays.add(espMarker)
        }
    }
    else{
            updatePositionMarker(lat,long,markerId,speed,true)
        }
        mapView.invalidate()
    }
    private fun markerExist(MarkerId:String):Boolean{
        return mapOverlays.overlays().any{ overlay->
            if(overlay is Marker)
                overlay.id==MarkerId
            else
                false
        }
    }
    private fun updatePositionMarker(lat:Double,long:Double,markerId:String,speed: Float,animated:Boolean){
        val newPos=GeoPoint(lat,long)
        lateinit var oldPos:GeoPoint

        mapOverlays.overlays().forEach {overlay ->
            if(overlay is Marker && overlay.id==markerId) {
                oldPos = overlay.position
                if (animated) {
                    val animator = ValueAnimator.ofFloat(0f, 1f)
                    animator.duration = 1000 // Duración de la animación en milisegundos
                    animator.addUpdateListener { animation ->
                        // Interpolación de las coordenadas
                        val fraction = animation.animatedFraction
                        val interpolatedLat =
                            oldPos.latitude + fraction * (newPos.latitude - oldPos.latitude)
                        val interpolatedLong =
                            oldPos.longitude + fraction * (newPos.longitude - oldPos.longitude)
                        overlay.apply {
                            position = GeoPoint(interpolatedLat, interpolatedLong)
                            if (markerId == "esp32")
                                snippet = "${speed} Km/h"
                            else if (markerId == "selectedLocation")
                                snippet =
                                    "${String.format("%.4f", lat)},${String.format("%.4f", long)}"
                        }
                        mapView.invalidate()
                    }
                    animator.start()
                }
                else{
                    overlay.apply {
                        position = GeoPoint(lat, long)
                        if (markerId == "esp32")
                            snippet = "${speed} Km/h"
                        else if (markerId == "selectedLocation")
                            snippet =
                                "${String.format("%.4f", lat)},${String.format("%.4f", long)}"
                    }
                    mapView.invalidate()
                }
            }
        }
    }
}

