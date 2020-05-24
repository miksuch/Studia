import javax.swing.*;
import java.io.*;
import java.net.Socket;

public class MyChatClient extends MyChat implements Runnable {

    private Socket socket;
    private BufferedReader socketReader;
    private PrintWriter socketWriter;
    private boolean verifiedWithServer;

    private MyChatUI myChatUI;

    MyChatClient( String username ) {
        hostname = username;
        myChatUI = null;
        verifiedWithServer = false;
        socket = null;
        socketWriter = null;
        socketReader = null;
    }

    public boolean connectWithServer( String servername, int port ){
        try {
            socket = new Socket(servername, port);
            socketReader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            socketWriter = new PrintWriter(socket.getOutputStream(), true);
            verifiedWithServer = false;
            return true;
        } catch ( IOException e ){
            return false;
        }
    }
    public void verifyWithServer() { // sending over username
        try {
            String serverOutput;
            socketWriter.println(hostname);
            serverOutput = socketReader.readLine();
            if (serverOutput.equals("ok")) {
                verifiedWithServer = true;
            } else {
                myChatUI.getMsgField().append( serverOutput );
                verifiedWithServer = false;
            }
        } catch ( IOException | NullPointerException e ) {
            verifiedWithServer = false;
        }
    }

    public void joinServer(){

        while( !verifiedWithServer ) {

            if( hostname.equals("") ) {
                hostname = myChatUI.askForUsername("user" + (int) (Math.random() * 100));
            }
            if( hostname == null ){
                hostname = "";
                if( myChatUI.exitDialog() == JOptionPane.YES_OPTION ) {
                    //possibly handling
                    System.exit(0);
                } else continue;
            }

            String[] servInfo = myChatUI.askForServerInfo( true );
            if( servInfo == null ){
                if( myChatUI.exitDialog() == JOptionPane.YES_OPTION ) {
                    System.exit(0);
                } else continue;
            }
            if( !connectWithServer( servInfo[0], Integer.parseInt(servInfo[1]) ) ){
                myChatUI.getMsgField().append("Could not establish connection with server.\n");
                continue;
            }

            verifyWithServer();
            if ( verifiedWithServer ) {
                myChatUI.getMsgField().append("You successfully joined the server.\n");
                return;
            } else {
                myChatUI.getMsgField().append("You failed to join the server.\n");
                hostname = "";
                continue;
            }
        }
    }

    // main thread
    @Override
    public void run() {
        // setting up ui
        myChatUI = new MyChatUI("MyChat Client");
        myChatUI.getInputField().addActionListener(e -> {
            String text = myChatUI.getInputField().getText();
            Message msgBuf = new Message(hostname, text );
            myChatUI.getMsgField().append( msgBuf + "\n" );
            myChatUI.getInputField().setText("");
            if(verifiedWithServer)
                socketWriter.println( msgBuf.getContent() );
        });

        joinServer();
        String serverOutput;
        while (true) {
            try {
                serverOutput = socketReader.readLine();
                if ( serverOutput != null ) {
                    myChatUI.getMsgField().append( serverOutput + '\n' );
                }
                else throw new IOException();
            } catch ( IOException | NullPointerException e ) {
                //e.printStackTrace();
                myChatUI.getMsgField().append("You were disconnected from server.\n");
                verifiedWithServer = false;
                joinServer();
                continue;
            }
        }
    }

    public static void main(String[] args) {
        MyChatClient myChatClientRunnableObj = new MyChatClient( "" );
        Thread myChatClient = new Thread( myChatClientRunnableObj, "myChatClient" );
        myChatClient.start();
    }
}
