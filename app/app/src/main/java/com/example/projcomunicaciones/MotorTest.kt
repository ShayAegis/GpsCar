package com.example.projcomunicaciones

import android.content.Intent
import android.os.Bundle
import android.view.MenuItem
import android.widget.SeekBar
import android.widget.TextView
import androidx.activity.viewModels
import androidx.appcompat.app.ActionBarDrawerToggle
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.GravityCompat
import androidx.drawerlayout.widget.DrawerLayout
import com.example.projcomunicaciones.viewmodels.MainActivityViewModel
import com.google.android.material.navigation.NavigationView

class MotorTest :AppCompatActivity(),NavigationView.OnNavigationItemSelectedListener  {
    private lateinit var toolbar:androidx.appcompat.widget.Toolbar
    private lateinit var motorPower:SeekBar
    lateinit var seekBarTV:TextView
    private lateinit var drawer: DrawerLayout
    private lateinit var toggle:ActionBarDrawerToggle
    private val vm: MainActivityViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.motors_test)
        motorPower = findViewById(R.id.motorSeekbar)
        seekBarTV = findViewById(R.id.motorTV)
        vm.connectMQTT()
        initToolbar()
        motorPower.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                if (fromUser) {
                    val seekbarTextvalue= getString(R.string.motor_display,progress)
                    seekBarTV.text = seekbarTextvalue
                }
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {

            }

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                seekBar?.progress?.let {
                    vm.sendCommandMQTT("motor",
                        it.toFloat(),"ufpsclient/sentCoords")
                }
            }
        })
    }
    override fun onStart() {
        super.onStart()
        vm.connectedStatus.observe(this){connected->
            if(connected)
                toolbar.title=getString(R.string.conected_mqtt)
            else
                toolbar.title=getString(R.string.disconected_mqtt)
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
    private fun goHome(){
        val intent= Intent(this,MainActivity::class.java)
        startActivity(intent)
    }

    override fun onNavigationItemSelected(item: MenuItem): Boolean {
        when(item.itemId){
            R.id.nav_map-> goHome()
            R.id.nav_compass_test -> callCompassTest()
        }
        drawer.closeDrawer(GravityCompat.START)
        return true
    }
    private fun callCompassTest(){
        intent= Intent(this,CompassTest::class.java)
        startActivity(intent)
    }
}


