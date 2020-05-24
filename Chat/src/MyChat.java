abstract public class MyChat {

    private static final int port = 2020;
    protected String hostname;

    public static int getPort() {
        return port;
    }
    public String getHostname() {
        return hostname;
    }
    public void setHostname(String hostname) {
        this.hostname = hostname;
    }
}



