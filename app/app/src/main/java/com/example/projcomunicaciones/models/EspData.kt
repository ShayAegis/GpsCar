package com.example.projcomunicaciones.models
import java.io.Serializable
import android.os.Parcelable
import com.google.gson.annotations.SerializedName
import kotlinx.parcelize.Parcelize

@Parcelize
data class EspData(
    @SerializedName("lat") protected val lat: Double,
    @SerializedName("long") protected val long: Double,
    @SerializedName("speed") protected val speed: Float,
    @SerializedName("orientation") protected val orientation: Float
) : Parcelable {
    fun fetchLat(): Double {
        return lat
    }

    fun fetchLong(): Double {
        return long
    }

    fun fetchSpeed(): Float {
        return speed
    }
    fun fetchOrientation(): Float {
        return orientation
    }
}
