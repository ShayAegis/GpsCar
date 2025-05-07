package com.example.projcomunicaciones

import android.content.Intent
import android.os.Bundle
import android.view.MenuItem
import android.widget.Button
import androidx.activity.viewModels
import androidx.appcompat.app.ActionBarDrawerToggle
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.GravityCompat
import androidx.drawerlayout.widget.DrawerLayout
import com.example.projcomunicaciones.viewmodels.MainActivityViewModel
import com.google.android.material.navigation.NavigationView

class CompassTest:AppCompatActivity(),NavigationView.OnNavigationItemSelectedListener {
    private val vm: MainActivityViewModel by viewModels()
    lateinit var toolbar:androidx.appcompat.widget.Toolbar
    lateinit var drawer: DrawerLayout
    lateinit var toggle: ActionBarDrawerToggle
    lateinit var arrayButtons: Array<Button>
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.compass_test)
        initToolbar()
        vm.connectMQTT()
        arrayButtons = arrayOf(
            findViewById(R.id.btn_align_north),
            findViewById(R.id.btn_align_south),
            findViewById(R.id.btn_align_west),
            findViewById(R.id.btn_align_east)
        )
    }

    override fun onStart() {
        super.onStart()
        vm.connectedStatus.observe(this){connected->
            if(connected)
                toolbar.title=getString(R.string.conected_mqtt)
            else
                toolbar.title=getString(R.string.disconected_mqtt)
        }
        arrayButtons.forEach {button:Button->
            button.setOnClickListener {
                val value=button.getTag().toString().toFloat()
                vm.sendOrientationMQTT(value,"ufpsclient/sentCoords")
            }
        }
    }
    fun initToolbar(){
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
        }
        drawer.closeDrawer(GravityCompat.START)
        return true
    }
    private fun goHome(){
        val intent= Intent(this,MainActivity::class.java)
        startActivity(intent)
    }
}