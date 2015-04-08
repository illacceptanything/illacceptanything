import java.io.*;
import java.net.URL;
import java.net.URLConnection;

public class AcceptAllPullRequestsTest implements IAcceptAllPullRequestsTestEnterpriceyTestAdaptorGeneratorFactoryHandler {

    public static void main(String[] args) throws IOException {
        URL url = new URL("https://api.github.com/repos/mrkrstphr/illacceptanything/pulls?state=open");
        URLConnection connection = url.openConnection();

        InputStream inputStream = connection.getInputStream();
        BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(inputStream));

        String response = bufferedReader.readLine();
        bufferedReader.close();

        if (response.equals("[]")) {
            System.out.println("Hooray!!! You've accepted everything!");
        } else {
            throw new RuntimeException("You've got unmerged pull requests! Get to mergin', slacker!");
        }

    }


}
