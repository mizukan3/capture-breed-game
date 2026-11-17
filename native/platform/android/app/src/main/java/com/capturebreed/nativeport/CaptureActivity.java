package com.capturebreed.nativeport;
import org.libsdl.app.SDLActivity;
public final class CaptureActivity extends SDLActivity {
    @Override protected String[] getLibraries() { return new String[] {"SDL3","main"}; }
}
