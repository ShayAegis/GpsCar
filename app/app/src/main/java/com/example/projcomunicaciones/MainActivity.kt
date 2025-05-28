package com.example.projcomunicaciones

import android.animation.ValueAnimator
import android.content.Intent
import android.os.Bundle
import android.view.MenuItem
import android.widget.ScrollView
import android.widget.TextView
import androidx.activity.viewModels
import androidx.appcompat.app.ActionBarDrawerToggle
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.GravityCompat
import androidx.drawerlayout.widget.DrawerLayout
import com.example.projcomunicaciones.viewmodels.MainActivityViewModel
import com.google.android.material.floatingactionbutton.FloatingActionButton
import com.google.android.material.navigation.NavigationView
import org.osmdroid.config.Configuration
import org.osmdroid.config.IConfigurationProvider
import org.osmdroid.events.MapEventsReceiver
import org.osmdroid.util.GeoPoint
import org.osmdroid.views.MapController
import org.osmdroid.views.MapView
import org.osmdroid.views.overlay.MapEventsOverlay
import org.osmdroid.views.overlay.Marker
import org.osmdroid.views.overlay.OverlayManager

class MainActivity : AppCompatActivity(), NavigationView.OnNavigationItemSelectedListener {
    private lateinit var publishBtn:FloatingActionButton
    lateinit var mapView:MapView
    private lateinit var mapController: MapController
    lateinit var mapOverlays:OverlayManager
    private lateinit var logsTV:TextView
    private lateinit var toolbar:androidx.appcompat.widget.Toolbar
    private lateinit var drawer:DrawerLayout
    private lateinit var toggle:ActionBarDrawerToggle
    var selectedLocation:GeoPoint?=null
    private val cucutaGeopoint=GeoPoint(7.889325709440754, -72.49674315409291)
    private val vm: MainActivityViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        initToolbar()
        publishBtn=findViewById(R.id.btPublish)
        logsTV=findViewById(R.id.tvEspLogs)
        loadMap(18,cucutaGeopoint)
        vm.connectMQTT()

    }

    override fun onStart() {
        super.onStart()
        publishBtn.setOnClickListener{
            selectedLocation?.let {
                vm.sendLocationMQTT(selectedLocation,"ufpsclient/sentCoords")
            }
        }
        vm.espLiveData.observe(this) { espData ->
            espData?.let {
                addPositionMarker(it.fetchLat(), it.fetchLong(), it.fetchSpeed())
            }
        }
        vm.espLiveLogs.observe(this) { logs ->
            val scrollview=findViewById<ScrollView>(R.id.scroll_view_logs)
            logsTV.append(logs)
            scrollview.smoothScrollTo(0,scrollview.getBottom())
        }
        vm.connectedStatus.observe(this,{connected->
            if(connected)
                toolbar.title=getString(R.string.conected_mqtt)
            else {
                toolbar.title=getString(R.string.disconected_mqtt)
            }
        })
    }

    private fun callCompassTest(){
        intent= Intent(this,CompassTest::class.java)
        startActivity(intent)
    }
    private fun callMotorTest(){
        intent= Intent(this,MotorTest::class.java)
        startActivity(intent)
    }
    private fun initToolbar(){
        toolbar = findViewById(R.id.main_toolbar)
        setSupportActionBar(toolbar)

        drawer=findViewById(R.id.drawer_layout)
        toggle=ActionBarDrawerToggle(this,drawer,toolbar,R.string.navigation_drawer_open,R.string.navigation_drawer_close)
        drawer.addDrawerListener(toggle)
        toggle.syncState()
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        supportActionBar?.setHomeAsUpIndicator(R.drawable.navigation_view_icon)
        initNavigationMenu()

    }
    private fun initNavigationMenu(){
        val navView:NavigationView = findViewById(R.id.nav_view)
        navView.setNavigationItemSelectedListener(this)
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
    override fun onNavigationItemSelected(item:MenuItem):Boolean{
        when(item.itemId){
            R.id.nav_compass_test -> callCompassTest()
            R.id.nav_motor_test -> callMotorTest()
        }
        drawer.closeDrawer(GravityCompat.START)
        return true
    }
}

