import javax.swing.*;
import java.awt.*;

public class MyChatUI {

    private final JFrame mainFrame;

    private JScrollPane msgPanel;
    private JTextArea msgField;
    private JTextField inputField;
    private JPanel mainPanel;

    MyChatUI( String name ) {
        mainPanel = new JPanel(new GridBagLayout());

        msgField = new JTextArea();
        msgField.setEditable(false);

        GridBagConstraints c = new GridBagConstraints();
        c.fill = GridBagConstraints.BOTH;
        c.gridx = 0;
        c.gridy = 0;
        c.gridheight = 1;
        c.weightx = 1.0;
        c.weighty = 1.0;
        Insets i = new Insets(15, 15, 15, 15);
        c.insets = i;

        msgPanel = new JScrollPane();
        msgPanel = new JScrollPane(msgField);
        mainPanel.add(msgPanel, c);

        inputField = new JTextField();
        inputField.setEnabled(true);
        inputField.setEditable(true);

        c.fill = GridBagConstraints.HORIZONTAL;
        //c.ipady = 20; // makes field taller
        c.gridx = 0;
        c.gridy = 5;
        c.gridheight = 1;
        c.weightx = 1.0;
        c.weighty = 0;
        c.insets.top = 0;
        mainPanel.add(inputField, c);

        mainFrame = new JFrame(name);
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        mainFrame.setPreferredSize(new Dimension(screenSize.width / 2, screenSize.height / 2));
        mainFrame.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
        mainFrame.add(mainPanel);
        mainFrame.pack();
        mainFrame.setVisible(true);
    }

    public String[] askForServerInfo( boolean clientMode ) {
        JPanel panel = new JPanel();
        JTextField addrField = new JTextField(10);;
        if( clientMode ) {
            panel.add(new JLabel("Address:"));
            addrField.setText("localhost");
            panel.add(addrField);
        }
        panel.add(new JLabel("Port:"));
        JTextField portField = new JTextField(4);
        portField.setText( String.valueOf( MyChat.getPort() ) );
        panel.add(portField);

        while (true) {
            if (JOptionPane.showConfirmDialog(mainFrame, panel, "Server info", JOptionPane.OK_CANCEL_OPTION) == JOptionPane.OK_OPTION) {
                if( (clientMode && (addrField.getText().equals("") || portField.getText().equals("")))
                        || (!clientMode && portField.getText().equals("")) ){
                    JOptionPane.showMessageDialog( mainFrame, "Server info can`t be empty", "Warning", JOptionPane.WARNING_MESSAGE );
                    continue;
                }
                else if( clientMode ) return new String[]{addrField.getText(), portField.getText()};
                else return new String[]{ "", portField.getText()};
            } else return null;
        }
    }

    public String askForUsername(String defaultUsername) {
        JPanel panel = new JPanel();
        panel.add(new JLabel("Username:"));
        JTextField nameField = new JTextField(10);
        nameField.setText(defaultUsername);
        panel.add(nameField);
        while (true) {
            if (JOptionPane.showConfirmDialog(mainFrame, panel, "User info", JOptionPane.OK_CANCEL_OPTION) == JOptionPane.OK_OPTION) {
                if( nameField.getText().equals("") ){
                    JOptionPane.showMessageDialog( mainFrame, "Username can`t be empty", "Warning", JOptionPane.WARNING_MESSAGE );
                    continue;
                } else return nameField.getText();
            } else return null;
        }
    }

    public int exitDialog() {
        return JOptionPane.showConfirmDialog(mainFrame,
                "Are you sure you want to exit?", "Exit warning",
                JOptionPane.YES_NO_OPTION, JOptionPane.WARNING_MESSAGE);
    }

    public JFrame getMainFrame() {
        return mainFrame;
    }

    public JTextField getInputField() {
        return inputField;
    }

    public JTextArea getMsgField() {
        return msgField;
    }

    public static void main(String[] args) {
        MyChatUI testUI = new MyChatUI( "MyChatUI");
    }
}
