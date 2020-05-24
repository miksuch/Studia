public class Message{

    private String sender;
    private String content;

    Message(String sender, String content ){
        this.sender = sender;
        this.content = content;
    }

    public String getContent() {
        return content;
    }

    public String getSender() {
        return sender;
    }

    public String toString(){
        return ( '['+sender+']' + ": " + content );
    }
}