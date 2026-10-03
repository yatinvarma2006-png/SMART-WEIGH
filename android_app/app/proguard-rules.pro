# Proguard rules for Smart Nutrition Scale app
-keepattributes *Annotation*
-keepclassmembers class * {
    @androidx.room.* <methods>;
}
