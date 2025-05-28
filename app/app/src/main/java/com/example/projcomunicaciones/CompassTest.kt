package com.example.projcomunicaciones

import android.content.Intent
import android.os.Bundle
import android.view.LayoutInflater
import android.view.MenuItem
import android.widget.Button
import android.widget.TextView
import androidx.activity.viewModels
import androidx.appcompat.app.ActionBarDrawerToggle
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.GravityCompat
import androidx.drawerlayout.widget.DrawerLayout
import com.example.projcomunicaciones.databinding.CompassTestLayoutBinding
import com.example.projcomunicaciones.viewmodels.MainActivityViewModel
import com.google.android.material.navigation.NavigationView

class CompassTest:AppCompatActivity(),NavigationView.OnNavigationItemSelectedListener {
    private val vm: MainActivityViewModel by viewModels()
    private lateinit var toolbar:androidx.appcompat.widget.Toolbar
    private lateinit var drawer: DrawerLayout
    private lateinit var toggle: ActionBarDrawerToggle
    private lateinit var arrayButtons: Array<Button>
    private lateinit var binding:CompassTestLayoutBinding
    val commandTopic="ufpsclient/sentCoords"
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding=CompassTestLayoutBinding.inflate(layoutInflater)
        setContentView(binding.root)
        initToolbar()
        vm.connectMQTT()
        arrayButtons = arrayOf(
            binding.btnAlignNorth,
            binding.btnAlignEast,
            binding.btnAlignWest,
            binding.btnAlignSouth
        )
        binding.btnCalibration.setOnClickListener { startCalibration() }
    }

    override fun onStart() {
        super.onStart()
        vm.connectedStatus.observe(this){connected->
            if(connected)
                toolbar.title=getString(R.string.conected_mqtt)
            else
                toolbar.title=getString(R.string.disconected_mqtt)
        }
        vm.espLiveData.observe(this){espData->
            val orientation=espData.fetchOrientation()
            val orientationPlaceholder=getString(R.string.azimuth_display, orientation)
            binding.tvOrientation.text = orientationPlaceholder
        }
        arrayButtons.forEach {button:Button->
            button.setOnClickListener {
                val value=button.tag.toString().toFloat()
                vm.sendCommandMQTT("orientation",value,commandTopic)
            }
        }
    }
    private fun initToolbar(){
        toolbar=findViewById(R.id.main_toolbar)
        setSupportActionBar(toolbar)
        drawer=findViewById(R.id.drawer_layout)
        toggle= ActionBarDrawerToggle(this,drawer,toolbar,R.string.navigation_drawer_open,R.string.navigation_drawer_close)
        drawer.addDrawerListener(toggle)
        toggle.syncState()
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        supportActionBar?.setHomeAsUpIndicator(R.drawable.navigation_view_icon)
        initNavigationMenu()
    }
    private fun initNavigationMenu(){
        val navView=findViewById<NavigationView>(R.id.nav_view)
        navView.setNavigationItemSelectedListener(this)
    }

    override fun onNavigationItemSelected(item: MenuItem): Boolean {
        when(item.itemId){
            R.id.nav_map-> goHome()
            R.id.nav_motor_test -> callMotorTest()
        }
        drawer.closeDrawer(GravityCompat.START)
        return true
    }
    private fun goHome(){
        val intent= Intent(this,MainActivity::class.java)
        startActivity(intent)
    }
    private fun callMotorTest(){
        intent= Intent(this,MotorTest::class.java)
        startActivity(intent)
    }
    private fun startCalibration(){
        vm.sendCommandMQTT("calibration",true,commandTopic)
    }
}