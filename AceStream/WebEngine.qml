import QtQuick 2.5
import QtQuick.Window 2.2
import QmlVlc 0.1
import QtWebEngine 1.2
import QtWebChannel 1.0

Window {
    color: 'grey';
    visible: true;
    width: 320;
    height: 240;

    VlcPlayer {
        id: player;
	mrl: "http://download.blender.org/peach/bigbuckbunny_movies/big_buck_bunny_480p_stereo.avi";
    }
    VlcVideoSurface {
        source: player;
        anchors.fill: parent;
    }
    WebEngineView {
        id: webView;
        url: "WebEngine.index.htm";
        anchors.fill: parent;
        backgroundColor: "transparent";
    }
    Component.onCompleted: {
        webView.webChannel.registerObject( 'player', player );
    }
}
