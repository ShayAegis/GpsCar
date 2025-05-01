package com.example.projcomunicaciones.repositories

import android.util.Log
import com.example.projcomunicaciones.models.EspData
import com.example.projcomunicaciones.viewmodels.MainActivityViewModel
import com.google.gson.Gson
import org.eclipse.paho.client.mqttv3.*
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence

class   MQTTRepository(private val serverUrl: String, private val viewModel: MainActivityViewModel) {
    private var client: MqttClient
    private val clientId = MqttClient.generateClientId()
    private lateinit var options: MqttConnectOptions
    private val esp32Coords:String="ufpsesp32/coords"
    private val esp32Logs:String="ufpsesp32/logs"
    init {
        val persistence = MemoryPersistence()
        client = MqttClient(serverUrl, clientId, persistence)

        client.setCallback(object : MqttCallbackExtended {
            override fun connectComplete(reconnect: Boolean, serverURI: String?) {
                Log.i("MQTT", "Conectado exitosamente a $serverURI (reconexión: $reconnect)")

                try {
                    // Suscripción automática al conectarse
                    client.subscribe(esp32Coords)
                    client.subscribe(esp32Logs)
                    Log.i("MQTT", "Suscripción exitosa al topic")
                } catch (e: MqttException) {
                    Log.e("MQTT", "Error al suscribirse al topic", e)
                }
            }

            override fun connectionLost(cause: Throwable?) {
                Log.e("MQTT", "Conexión perdida", cause)
            }

            override fun messageArrived(topic: String?, message: MqttMessage?) {
                val json=message.toString()
                Log.i("MQTT","Message Received $topic")
                when(topic){
                    esp32Coords-> {
                        val espData=Gson().fromJson(json,EspData::class.java)
                        viewModel.updateLocation(espData)
                    }
                    esp32Logs-> viewModel.updateLogs(json)
                }
            }

            override fun deliveryComplete(token: IMqttDeliveryToken?) {
                Log.i("MQTT", "Publicación completada")
            }
        })
    }

    fun mqttConnect():Boolean {
        return try {
            options = MqttConnectOptions().apply {
                isCleanSession = true
                connectionTimeout = 10
                keepAliveInterval = 30
            }

            if (!client.isConnected) {
                client.connect(options)
                Log.i("MQTT", "Intentando conectar a $serverUrl")
            }
            client.isConnected
        } catch (e: MqttException) {
            Log.e("MQTT", "Error al conectar", e)
            false
        }
    }

    fun publishMessage(topic: String, message: String): Boolean {
        var publishResult = false
        try {
            if (client.isConnected) {
                val msg = MqttMessage(message.toByteArray(Charsets.UTF_8)).apply {
                    qos = 0
                }
                client.publish(topic, msg)
                publishResult = true
                Log.i("MQTT", "Mensaje publicado en $topic: $message")
            } else {
                Log.e("MQTT", "No conectado a MQTT, no se puede publicar.")
            }
        } catch (e: MqttException) {
            Log.e("MQTT", "Error al publicar mensaje", e)
        }
        return publishResult
    }

    fun disconnect() {
        try {
            if (client.isConnected) {
                client.disconnect()
                Log.i("MQTT", "Desconectado del broker.")
            } else {
                Log.w("MQTT", "Intento de desconexión cuando ya estaba desconectado.")
            }
        } catch (e: MqttException) {
            Log.e("MQTT", "Error al desconectar", e)
        }
    }
}
