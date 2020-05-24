import javax.swing.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.concurrent.Semaphore;

public class MyChatServer extends MyChat implements Runnable {

    private ServerSocket serverSocket;
    private Semaphore connectedClientsMutex; // for blocking during r/w in threads
    private LinkedList<MyChatClientHandler> connectedClients;
    private boolean serverIsOpen; // threads check for this to return nicely
    private int clientNumber; // for identification of every client connected even if he didnt complete verifying ( sending over viable username )

    private MessageSender messageSender;
    private Thread clientsCleanup;

    private MyChatUI myChatUI;

    MyChatServer(String serverName ) throws IOException {

        serverSocket = new ServerSocket();
        serverSocket.setSoTimeout(2000);
        connectedClientsMutex = new Semaphore(3);
        connectedClients = new LinkedList<>();
        serverIsOpen = false;
        clientNumber = 0;
        messageSender = new MessageSender();
        clientsCleanup = new Thread( new InactiveClientsCleanup(), "ClientsCleanup" );
        hostname = serverName;
        myChatUI = null;
    }

    // main thread
    @Override
    public void run() {
        // setting up ui
        myChatUI = new MyChatUI( "MyChat Server");
        myChatUI.getMainFrame().setDefaultCloseOperation(WindowConstants.DO_NOTHING_ON_CLOSE);
        myChatUI.getMainFrame().addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                if( myChatUI.exitDialog() == JOptionPane.YES_OPTION ) {
                    closeServer();
                }
            }
        });
        myChatUI.getInputField().addActionListener(e -> {
            String text = myChatUI.getInputField().getText();
            Message msgBuf = new Message(hostname, text );
            myChatUI.getInputField().setText("");
            messageSender.sendMessage( msgBuf );
        });

        // setting up server
        while( !serverIsOpen ) {

            String[] servInfo = myChatUI.askForServerInfo(false);
            if (servInfo == null) {
                if (myChatUI.exitDialog() == JOptionPane.YES_OPTION) {
                    closeServer();
                    break;
                } else continue;
            } else {
                try {
                    serverSocket = new ServerSocket(Integer.parseInt(servInfo[1]));
                    serverSocket.setSoTimeout(2000);
                } catch (IOException | IllegalArgumentException e) {
                    JOptionPane.showMessageDialog( myChatUI.getMainFrame(), "Could not start server on port " + servInfo[1] );
                    myChatUI.getMsgField().append(Arrays.toString(e.getStackTrace()));
                    closeServer();
                    break;
                }
                serverIsOpen = true;
                myChatUI.getMsgField().append("Server started.\n");
            }
        }
        messageSender.start();
        clientsCleanup.start();

        // main loop
        while (serverIsOpen) {
            try {
                MyChatClientHandler newClient = new MyChatClientHandler( serverSocket.accept() );
                newClient.start();
                connectedClients.addLast( newClient ); // no need to block for adding
            } catch (SocketTimeoutException e) {
                continue;
            } catch (IOException e) {
                myChatUI.getMsgField().append(Arrays.toString(e.getStackTrace()));
                myChatUI.getMsgField().append(String.valueOf(new Message(hostname, "accept(): I/O Error\n")));
                continue;
            }
            myChatUI.getMsgField().append(String.valueOf(new Message(hostname, "Client " + connectedClients.getLast().getClientId() + " added.\n")));
        }
        // closing server
        try {
            clientsCleanup.interrupt();
            clientsCleanup.join();
            messageSender.join();
            while (!connectedClients.isEmpty()) {
                connectedClients.pollFirst().close();
            }
            serverSocket.close();
        } catch (InterruptedException | IOException e) {
            myChatUI.getMsgField().append(Arrays.toString(e.getStackTrace()));
        }
        myChatUI.getMainFrame().dispose();
        return;
    }

    public class MessageSender extends Thread {

        private LinkedList<Message> messagesQueue = new LinkedList<>();

        public void sendMessage(Message msg) {
            messagesQueue.addLast(msg);
        }

        @Override
        public void run() {

            Message msgToSend;
            while (true) {
                try {
                    connectedClientsMutex.acquire(1); // can use it along with client handler ( while its verifying )
                    msgToSend = messagesQueue.pollFirst();

                    if (!serverIsOpen) {
                        msgToSend = new Message(hostname, "Shutting down...");
                    }

                    if (msgToSend != null) {

                        // echo for server
                        myChatUI.getMsgField().append(msgToSend + "\n");

                        for (MyChatClientHandler receiver : connectedClients) {
                            if (receiver.isClientAvailable() && !receiver.getClientName().equals(msgToSend.getSender())) {
                                receiver.getClientSocketWriter().println(msgToSend);
                            }
                        }

                        if (!serverIsOpen) {
                            return;
                        }
                    }

                } catch (InterruptedException e) {
                    if( !serverIsOpen )
                        return;
                } finally {
                    connectedClientsMutex.release(1);
                }
            }
        }
    }

    public class MyChatClientHandler extends Thread implements Closeable {

        private final String id;
        private boolean clientAvailable;
        private boolean clientVerifying;
        private String clientName;
        private BufferedReader clientSocketReader;
        private PrintWriter clientSocketWriter;
        private Socket clientSocket;

        MyChatClientHandler(Socket s) throws IOException {
            clientVerifying = true;
            clientAvailable = false;
            id = "#" + (++clientNumber);
            clientSocket = s;
            clientSocketReader = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
            clientSocketWriter = new PrintWriter(clientSocket.getOutputStream(), true);
        }

        @Override
        public void run() {

            String message;
            // verifying
            try {

                connectedClientsMutex.acquire(2); // can use it along with message sender, but only one client can verify at once

                clientName = clientSocketReader.readLine();
                if (clientName != null) {
                    for (MyChatClientHandler client : connectedClients) {
                        if ( (clientName.equals(client.getClientName()) && client.isClientAvailable()) || clientName.equals( hostname ) ) {
                            clientAvailable = false;
                            myChatUI.getMsgField().append(String.valueOf(new Message(hostname, "Failed to verify client " + this.getClientId() + ".\n")));
                            clientSocketWriter.println(new Message(hostname,
                                    "This username is taken, choose a different one."));
                            return;
                        }
                    }
                    clientAvailable = true;
                    clientSocketWriter.println("ok");
                    myChatUI.getMsgField().append(String.valueOf(new Message(hostname, "Successfully verified client " + this.getClientId() + ".\n")));
                    messageSender.sendMessage(new Message(hostname,
                            clientName + " has joined the server."));

                } else throw new IOException();

            } catch (InterruptedException | IOException e) {
                clientAvailable = false;
                myChatUI.getMsgField().append(String.valueOf(new Message(hostname, "Failed to verify client " + this.getClientId() + ".\n")));
                return;
            } finally {
                clientVerifying = false;
                connectedClientsMutex.release(2);
            }
            // end of verifying, main loop
            while (true) {
                try {
                    message = clientSocketReader.readLine();
                } catch (IOException | NullPointerException e) { // sets up for disconnecting with client
                    clientAvailable = false;
                    message = null;
                }
                if (message != null) {
                    messageSender.sendMessage(new Message(clientName, message));
                } else {
                    clientAvailable = false;
                    messageSender.sendMessage(new Message(hostname,
                            clientName + " has left the server."));
                    return;
                }
            }
        }

        @Override
        public void close() throws IOException {
            clientAvailable = false;
            clientSocket.close();
            clientSocketWriter.close();
            clientSocketReader.close();
        }

        public PrintWriter getClientSocketWriter() {
            return clientSocketWriter;
        }

        public String getClientId() {
            return id;
        }

        public String getClientName() {
            return clientName;
        }

        public boolean isClientVerifying() {
            return clientVerifying;
        }

        public boolean isClientAvailable() {
            return clientAvailable;
        }
    }

    private class InactiveClientsCleanup implements Runnable {
        @Override
        public void run() {
            while (true) {
                try {
                    connectedClientsMutex.acquire(3);
                    // closing connections and resources
                    for (MyChatClientHandler connectedClient : connectedClients)
                        if (!connectedClient.isClientAvailable() && !connectedClient.isClientVerifying()) {
                            try {
                                connectedClient.close();
                            } catch (IOException e) {
                                myChatUI.getMsgField().append(String.valueOf(new Message(hostname, "Error closing connection with client " + connectedClient.getClientId() + ".\n")));
                            }
                        }
                    // removing
                    connectedClients.removeIf(c -> (!c.isClientAvailable() && !c.isClientVerifying()));

                    if (!serverIsOpen)
                        return;

                } catch (InterruptedException e) {
                    if (!serverIsOpen)
                        return;
                } finally {
                    connectedClientsMutex.release(3);
                }

                // waiting for another cleanup run
                try {
                    Thread.sleep(10000);
                } catch (InterruptedException e) {
                    if (!serverIsOpen)
                        return;
                }
            }
        }
    }

    public void closeServer() {
        serverIsOpen = false;
    }

    public static void main(String[] args) throws IOException {

        MyChatServer myChatServerRunnableObj = new MyChatServer("SERVER" );
        Thread myChatServer = new Thread(myChatServerRunnableObj, "myChatServer");
        myChatServer.start();
    }

}
